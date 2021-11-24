#include <aws/core/Aws.h>
#include <aws/core/utils/Outcome.h> 
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/dynamodb/DynamoDBClient.h>
#include <aws/dynamodb/model/AttributeDefinition.h>
#include <aws/dynamodb/model/GetItemRequest.h>
#include <aws/dynamodb/model/PutItemRequest.h>
#include <aws/dynamodb/model/PutItemResult.h>
#include <aws/lambda-runtime/runtime.h>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <cstdint>
#include <iostream>


using namespace aws::lambda_runtime;
char const TAG[] = "LAMBDA_ALLOC";

uint64_t upload_random_file(Aws::String const &table,
                        Aws::String const &key,
                        int size);

uint64_t download_file(Aws::String const &table,
                        Aws::String const &key,
                        int &required_retries,
                        bool report_dl_time);

uint64_t timeSinceEpochMillisec()
{
  auto now = std::chrono::high_resolution_clock::now();
  auto time = now.time_since_epoch();
  return std::chrono::duration_cast< std::chrono::microseconds >(time).count();
}


static invocation_response my_handler(invocation_request const &req)
{
    using namespace Aws::Utils::Json;
    JsonValue json(req.payload);
    if (!json.WasParseSuccessful())
    {
        return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
    }

    auto v = json.View();

    auto table = v.GetString("table");
    auto key = v.GetString("key");
    auto role = v.GetString("role"); // producer or consumer
    auto file_size = v.GetInteger("fileSize");
    bool report_dl_time = false;
    if (v.KeyExists("downloadTime")) {
        report_dl_time = true;
    }
    std::cout << "Invoked handler for role " << role << " with file size " << file_size << std::endl;

    std::string res_json = "{ \"fileSize\": " + std::to_string(file_size) + ", \"role\": \"" + role + "\"" ;
    if (role == "producer")
    {
        uint64_t upload_time = upload_random_file(table, key, file_size);
        res_json += ", \"uploadTime\": " + std::to_string(upload_time) + " }";
    }
    else if (role == "consumer")
    {
        int retries = 0;
        uint64_t finished_time = download_file(table, key, retries, report_dl_time);
        res_json += ", \"finishedTime\": " + std::to_string(finished_time) + ", \"retries\":" + std::to_string(retries) + " }";
    }
    
    return invocation_response::success(res_json, "application/json");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory()
{
    return []
    {
        return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
            "console_logger", Aws::Utils::Logging::LogLevel::Info);
    };
}

int main()
{
    using namespace Aws;
    SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
    InitAPI(options);
    {

        auto handler_fn = [](aws::lambda_runtime::invocation_request const &req)
        {
            return my_handler(req);
        };
        run_handler(handler_fn);
    }
    ShutdownAPI(options);
    return 0;
}

uint64_t download_file(Aws::String const &table,
                        Aws::String const &key,
                        int &required_retries,
                        bool report_dl_time)
{
    Aws::Client::ClientConfiguration clientConfig;
    Aws::DynamoDB::DynamoDBClient dynamoClient(clientConfig);
    Aws::DynamoDB::Model::GetItemRequest req;

    // Set up the request
    req.SetTableName(table);
    Aws::DynamoDB::Model::AttributeValue hashKey;
    hashKey.SetS(key);
    req.AddKey("size", hashKey);

    auto bef = timeSinceEpochMillisec();
    // Retrieve the item's fields and values
    const Aws::DynamoDB::Model::GetItemOutcome& result = dynamoClient.GetItem(req);
    if (result.IsSuccess()) {
        // Reference the retrieved fields/values
        const Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>& item = result.GetResult().GetItem();
        if (item.size() > 0) {
            auto after = timeSinceEpochMillisec();
            for (const auto& i : item)
                std::cout << i.first << std::endl;
            return after - bef;
        } else {
            std::cout << "No item found with the key " << key << std::endl;
        }

    } else {
        std::cout << "Failed to get item: " << result.GetError().GetMessage();
    }


}

uint64_t upload_random_file(Aws::String const &table,
                        Aws::String const &key,
                        int size)
{
    const unsigned char* pBuf = new unsigned char[size];
    Aws::Utils::ByteBuffer buf(pBuf, size);

    Aws::Client::ClientConfiguration clientConfig;
    Aws::DynamoDB::DynamoDBClient dynamoClient(clientConfig);

    Aws::DynamoDB::Model::PutItemRequest pir;
    pir.SetTableName(table);

    Aws::DynamoDB::Model::AttributeValue av;
    av.SetB(buf);
    pir.AddItem("data", av);
    av.SetS(key);
    pir.AddItem("size", av);

    const Aws::DynamoDB::Model::PutItemOutcome result = dynamoClient.PutItem(pir);
    if (!result.IsSuccess()) {
        std::cout << result.GetError().GetMessage() << std::endl;
        return 1;
    }
    std::cout << "Done!" << std::endl;
}