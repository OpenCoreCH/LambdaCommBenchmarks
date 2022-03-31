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
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
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

uint64_t timeSinceEpochMillisec()
{
  auto now = std::chrono::high_resolution_clock::now();
  auto time = now.time_since_epoch();
  return std::chrono::duration_cast< std::chrono::microseconds >(time).count();
}

uint64_t upload_random_file(Aws::DynamoDB::DynamoDBClient& dynamoClient,
                        Aws::String const &table,
                        Aws::String const &key,
                        int size,unsigned char*);

uint64_t download_file(Aws::DynamoDB::DynamoDBClient& dynamoClient,
                        Aws::String const &table,
                        Aws::String const &key,
                        int &required_retries,
                        bool report_dl_time);

uint64_t upload_random_file_s3(Aws::S3::S3Client const &client,
                        Aws::String const &bucket,
                        Aws::String const &key,
                        int size, char* pBuf)
{
    /**
     * We allocate and do not initialize memory on purpose to get "random" data.
     * Explicitly generating random data in a 256MB Lambda Function and passing it to a stringstream 
     * (i.e. the way that is documented here: https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/examples-s3-objects.html)
     * takes way too long, ~55s for 100MB
     */

    /**
     * We use Boost's bufferstream to wrap the array as an IOStream. Usign a light-weight streambuf wrapper, as many solutions 
     * (e.g. https://stackoverflow.com/questions/13059091/creating-an-input-stream-from-constant-memory) on the internet
     * suggest does not work because the S3 SDK relies on proper functioning tellp(), etc... (for instance to get the body length).
     */
    const std::shared_ptr<Aws::IOStream> input_data = std::make_shared<boost::interprocess::bufferstream>(pBuf, size);

    Aws::S3::Model::PutObjectRequest request;
    request.WithBucket(bucket).WithKey(key);
    request.SetBody(input_data);
    uint64_t bef_upload = timeSinceEpochMillisec();
    Aws::S3::Model::PutObjectOutcome outcome = client.PutObject(request);
    if (!outcome.IsSuccess()) {
        std::cout << "Error: PutObject: " << 
            outcome.GetError().GetMessage() << std::endl;
    }
    //delete[] pBuf;
    return bef_upload;
}


static invocation_response my_handler(invocation_request const &req, Aws::S3::S3Client const &client,Aws::DynamoDB::DynamoDBClient& dynamoClient)
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

    std::string bucket = "s3benchmark-eu";

    unsigned char* pBuf = new unsigned char[file_size];
    memset(pBuf, 'A', sizeof(unsigned char)*file_size);

    std::vector<uint64_t> times;
    std::vector<uint64_t> retries_times;
    int warmup_reps = 10;
    int reps = 1000;
    //int warmup_reps = 1;
    //int reps = 2;
    int retries = 0;
    if (role == "producer") {

      for(int i = 0; i < warmup_reps; ++i) {

        std::string new_key = key + "_" + std::to_string(i);
        std::string new_key_response = key + "_" + std::to_string(i) + "_response";
        upload_random_file(dynamoClient, table, new_key, file_size, pBuf);
        int ret = download_file(dynamoClient, table, new_key_response, retries, report_dl_time);
        if(ret == 0) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }
      } 

      for(int i = warmup_reps; i < reps + warmup_reps; ++i) {

        std::string new_key = key + "_" + std::to_string(i);
        std::string new_key_response = key + "_" + std::to_string(i) + "_response";
        auto beg = timeSinceEpochMillisec();
        upload_random_file(dynamoClient, table, new_key, file_size, pBuf);
        int ret = download_file(dynamoClient, table, new_key_response, retries, report_dl_time);
        auto end = timeSinceEpochMillisec();
        times.push_back(end - beg);
        retries_times.push_back(retries);

        if(ret == 0) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }
      } 

      std::stringstream ss;
      ss << times.size() << '\n';
      for(int i = 0; i < times.size(); ++i)
        ss << times[i] << '\n';
      std::stringstream ss2;
      ss2 << retries_times.size() << '\n';
      for(int i = 0; i < retries_times.size(); ++i)
        ss2 << retries_times[i] << '\n';

      auto times_str = ss.str();
      char* data = new char[times_str.length()];
      strcpy(data, times_str.c_str());

      auto retries_times_str = ss2.str();
      char* data2 = new char[retries_times_str.length()];
      strcpy(data2, retries_times_str.c_str());

      std::string new_key = key + "_" + std::to_string(file_size)+ "_producer_times";
      upload_random_file_s3(client, bucket, new_key, times_str.length(), data);
      new_key = key + "_" + std::to_string(file_size)+ "_producer_retries_times";
      upload_random_file_s3(client, bucket, new_key, retries_times_str.length(), data2);

      delete[] data;
      delete[] data2;
    } else if (role == "consumer") {

      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
      for(int i = 0; i < warmup_reps; ++i) {

        std::string new_key = key + "_" + std::to_string(i);
        std::string new_key_response = key + "_" + std::to_string(i) + "_response";
        int ret = download_file(dynamoClient, table, new_key, retries, report_dl_time);
        upload_random_file(dynamoClient, table, new_key_response, file_size, pBuf);
        if(ret == 0) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }
      } 

      for(int i = warmup_reps; i < reps + warmup_reps; ++i) {

        std::string new_key = key + "_" + std::to_string(i);
        std::string new_key_response = key + "_" + std::to_string(i) + "_response";
        int ret = download_file(dynamoClient, table, new_key, retries, report_dl_time);
        upload_random_file(dynamoClient, table, new_key_response, file_size, pBuf);
        retries_times.push_back(retries);
        if(ret == 0) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }

      } 

      std::stringstream ss2;
      ss2 << retries_times.size() << '\n';
      for(int i = 0; i < retries_times.size(); ++i)
        ss2 << retries_times[i] << '\n';
      auto retries_times_str = ss2.str();
      char* data = new char[retries_times_str.length()];
      strcpy(data, retries_times_str.c_str());
      std::string new_key = key + "_" + std::to_string(file_size) + "_consumer_retries_times";
      upload_random_file_s3(client, bucket, new_key, retries_times_str.length(), data);
      delete[] data;
    }

    std::string res_json = "{ \"fileSize\": " + std::to_string(file_size) + "}";
    delete[] pBuf;
    
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
    //options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;
    //options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
    InitAPI(options);
    {
        Client::ClientConfiguration config;
        config.region = "eu-central-1";
        config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";

        auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
        S3::S3Client client(credentialsProvider, config);
        //Aws::Client::ClientConfiguration clientConfig;
        Aws::DynamoDB::DynamoDBClient dynamoClient(config);
        auto handler_fn = [&dynamoClient, &client](aws::lambda_runtime::invocation_request const &req)
        {
            return my_handler(req, client, dynamoClient);
        };
        run_handler(handler_fn);
    }
    ShutdownAPI(options);
    return 0;
}

uint64_t download_file(Aws::DynamoDB::DynamoDBClient& dynamoClient,
                        Aws::String const &table,
                        Aws::String const &key,
                        int &required_retries,
                        bool report_dl_time)
{
    Aws::DynamoDB::Model::GetItemRequest req;

    // Set up the request
    req.SetTableName(table);
    Aws::DynamoDB::Model::AttributeValue hashKey;
    hashKey.SetS(key);
    req.AddKey("size", hashKey);

    auto bef = timeSinceEpochMillisec();

    int retries = 0;
    //const int MAX_RETRIES = 500;
    const int MAX_RETRIES = 1500;
    while (retries < MAX_RETRIES) {
        const Aws::DynamoDB::Model::GetItemOutcome& result = dynamoClient.GetItem(req);
        if (result.IsSuccess()) {
            // Reference the retrieved fields/values
            const Aws::Map<Aws::String, Aws::DynamoDB::Model::AttributeValue>& item = result.GetResult().GetItem();
            if (item.size() > 0) {
                auto after = timeSinceEpochMillisec();
                return after - bef;
            }
        } else {
            retries += 1;
        }
    }
    return 0;
}

uint64_t upload_random_file(Aws::DynamoDB::DynamoDBClient& dynamoClient,
                        Aws::String const &table,
                        Aws::String const &key,
                        int size, unsigned char* pBuf)
{
    Aws::Utils::ByteBuffer buf(pBuf, size);


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
    //std::cout << "Done!" << std::endl;
    return 0;
}
