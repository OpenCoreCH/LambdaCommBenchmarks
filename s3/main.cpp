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
#include <boost/interprocess/streams/bufferstream.hpp>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <cstdint>
#include <iostream>


using namespace aws::lambda_runtime;
char const TAG[] = "LAMBDA_ALLOC";

uint64_t upload_random_file(Aws::S3::S3Client const &client,
                        Aws::String const &bucket,
                        Aws::String const &key,
                        int size);

uint64_t download_file(Aws::S3::S3Client const &client,
                        Aws::String const &bucket,
                        Aws::String const &key,
                        int &required_retries,
                        bool report_dl_time);

uint64_t timeSinceEpochMillisec()
{
  auto now = std::chrono::high_resolution_clock::now();
  auto time = now.time_since_epoch();
  return std::chrono::duration_cast< std::chrono::microseconds >(time).count();
}


static invocation_response my_handler(invocation_request const &req, Aws::S3::S3Client const &client)
{
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
    bool report_dl_time = false;
    if (v.KeyExists("waitUntil")) {
        auto wait_until = v.GetInteger("waitUntil");
        while (std::time(0) < wait_until) {
            
        }

    }
    if (v.KeyExists("downloadTime")) {
        report_dl_time = true;
    }
    std::cout << "Invoked handler for role " << role << " with file size " << file_size << std::endl;

    std::string res_json = "{ \"fileSize\": " + std::to_string(file_size) + ", \"role\": \"" + role + "\"" ;
    if (role == "producer")
    {
        uint64_t upload_time = upload_random_file(client, bucket, key, file_size);
        res_json += ", \"uploadTime\": " + std::to_string(upload_time) + " }";
    }
    else if (role == "consumer")
    {
        int retries;
        uint64_t finished_time = download_file(client, bucket, key, retries, report_dl_time);
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
        Client::ClientConfiguration config;
        config.region = "eu-central-1";

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
                        Aws::String const &key,
                        int &required_retries,
                        bool report_dl_time)
{
    

    Aws::S3::Model::GetObjectRequest request;
    request.WithBucket(bucket).WithKey(key);
    auto bef = timeSinceEpochMillisec();

    int retries = 0;
    const int MAX_RETRIES = 500;
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
            required_retries = retries;
            if (report_dl_time) {
                return finishedTime - bef;
            } else {
                return finishedTime;
            }
        } else {
            retries += 1;
            int sleep_time = retries;
            if (retries > 100) {
                sleep_time = retries * 2;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
    }
    return 0;

}

uint64_t upload_random_file(Aws::S3::S3Client const &client,
                        Aws::String const &bucket,
                        Aws::String const &key,
                        int size)
{
    /**
     * We allocate and do not initialize memory on purpose to get "random" data.
     * Explicitly generating random data in a 256MB Lambda Function and passing it to a stringstream 
     * (i.e. the way that is documented here: https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/examples-s3-objects.html)
     * takes way too long, ~55s for 100MB
     */
    char* pBuf = new char[size];

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
    delete[] pBuf;
    return bef_upload;
}