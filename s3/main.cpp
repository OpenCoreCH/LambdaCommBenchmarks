#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/utils/HashingUtils.h>
#include <aws/core/platform/Environment.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/lambda-runtime/runtime.h>
#include <vector>
#include <random>
#include <chrono>
#include <cstdint>
#include <iostream>


using namespace aws::lambda_runtime;
using random_bytes_engine = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, unsigned char>;
char const TAG[] = "LAMBDA_ALLOC";

uint64_t upload_random_file(Aws::S3::S3Client const &client,
                        Aws::String const &bucket,
                        Aws::String const &key,
                        int size);

uint64_t download_file(Aws::S3::S3Client const &client,
                        Aws::String const &bucket,
                        Aws::String const &key);

uint64_t timeSinceEpochMillisec()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}


static invocation_response my_handler(invocation_request const &req, Aws::S3::S3Client const &client)
{
    std::cout << "Invoked handler" << std::endl;
    using namespace Aws::Utils::Json;
    JsonValue json(req.payload);
    if (!json.WasParseSuccessful())
    {
        return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
    }

    auto v = json.View();

    auto bucket = v.GetString("s3bucket");
    auto key = v.GetString("s3key");
    auto role = v.GetString("role"); // producer or consumer
    auto file_size = v.GetInteger("fileSize");

    std::string res_json = "{ \"fileSize\": " + std::to_string(file_size) + ", \"role\": \"" + role + "\"" ;
    if (role == "producer")
    {
        uint64_t upload_time = upload_random_file(client, bucket, key, file_size);
        res_json += ", \"uploadTime\": " + std::to_string(upload_time) + " }";
    }
    else if (role == "consumer")
    {
        uint64_t finished_time = download_file(client, bucket, key);
        res_json += ", \"finishedTime\": " + std::to_string(finished_time) + " }";
    }
    
    return invocation_response::success(res_json, "application/json");
}

std::function<std::shared_ptr<Aws::Utils::Logging::LogSystemInterface>()> GetConsoleLoggerFactory()
{
    return []
    {
        return Aws::MakeShared<Aws::Utils::Logging::ConsoleLogSystem>(
            "console_logger", Aws::Utils::Logging::LogLevel::Trace);
    };
}

int main()
{
    using namespace Aws;
    SDKOptions options;
    options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
    options.loggingOptions.logger_create_fn = GetConsoleLoggerFactory();
    InitAPI(options);
    {
        Client::ClientConfiguration config;
        config.region = Aws::Environment::GetEnv("AWS_REGION");
        config.caFile = "/etc/pki/tls/certs/ca-bundle.crt";

        auto credentialsProvider = Aws::MakeShared<Aws::Auth::EnvironmentAWSCredentialsProvider>(TAG);
        S3::S3Client client(credentialsProvider, config);
        auto handler_fn = [&client](aws::lambda_runtime::invocation_request const &req)
        {
            return my_handler(req, client);
        };
        run_handler(handler_fn);
    }
    ShutdownAPI(options);
    return 0;
}

uint64_t download_file(Aws::S3::S3Client const &client,
                        Aws::String const &bucket,
                        Aws::String const &key)
{
    

    Aws::S3::Model::GetObjectRequest request;
    request.WithBucket(bucket).WithKey(key);

    int retries = 0;
    const int MAX_RETRIES = 100;
    while (retries < MAX_RETRIES) {
        auto outcome = client.GetObject(request);
        if (outcome.IsSuccess()) {
            auto& s = outcome.GetResult().GetBody();
            uint64_t finishedTime = timeSinceEpochMillisec();
            // Perform NOP on result to prevent optimizations
            std::stringstream ss;
            ss << s.rdbuf();
            std::string first(" ");
            ss.get(&first[0], 1);
            return finishedTime;
        } else {
            retries += 1;
        }
    }
    return 0;

}

uint64_t upload_random_file(Aws::S3::S3Client const &client,
                        Aws::String const &bucket,
                        Aws::String const &key,
                        int size)
{
    random_bytes_engine rbe;
    std::vector<char> data(size);
    std::generate(begin(data), end(data), std::ref(rbe));

    Aws::S3::Model::PutObjectRequest request;
    request.WithBucket(bucket).WithKey(key);
    std::string s(data.begin(), data.end());
    const std::shared_ptr<Aws::IOStream> input_data =
        Aws::MakeShared<Aws::StringStream>("");
    *input_data << s.c_str();
    request.SetBody(input_data);
    uint64_t bef_upload = timeSinceEpochMillisec();
    Aws::S3::Model::PutObjectOutcome outcome = client.PutObject(request);
    return bef_upload;
}