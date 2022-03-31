#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/lambda-runtime/runtime.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <cstdint>
#include <iostream>
#include <hiredis/hiredis.h>


using namespace aws::lambda_runtime;
char const TAG[] = "LAMBDA_ALLOC";

uint64_t upload_random_file(redisContext* context,
                        std::string const &key,
                        int size, char *pBuf);

uint64_t download_file(redisContext* context,
                       std::string const &key,
                       bool report_dl_time);
uint64_t delete_file(redisContext* context,
                        std::string const &key);

uint64_t timeSinceEpochMillisec()
{
  auto now = std::chrono::high_resolution_clock::now();
  auto time = now.time_since_epoch();
  return std::chrono::duration_cast< std::chrono::microseconds >(time).count();
}

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

    std::cerr << "Upload result to: " << bucket << " key " << key << '\n';
    Aws::S3::Model::PutObjectRequest request;
    request.WithBucket(bucket).WithKey(key);
    request.SetBody(input_data);
    uint64_t bef_upload = timeSinceEpochMillisec();
    Aws::S3::Model::PutObjectOutcome outcome = client.PutObject(request);
    if (!outcome.IsSuccess()) {
        std::cerr << "Error: PutObject: " << 
            outcome.GetError().GetMessage() << std::endl;
    }
    //delete[] pBuf;
    return bef_upload;
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

    auto key = v.GetString("key");
    std::string redis_hostname = v.GetString("hostname");
    auto redis_port = v.GetInteger("port");
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
    std::string bucket = "s3benchmark-eu";

    redisContext *c = redisConnect(redis_hostname.c_str(), redis_port);
    if (c == NULL || c->err) {
        if (c) {
            printf("Redis Error: %s\n", c->errstr);
        } else {
            printf("Can't allocate redis context\n");
        }
    }

    std::vector<uint64_t> times;
    int warmup_reps = 10;
    int reps = 100;
    //int reps = 100;
    //int warmup_reps = 1;
    //int reps = 2;
    int retries = 0;

    char* recv_buffer = new char[file_size];
    char* pBuf = new char[file_size];
    memset(pBuf, 1, sizeof(char)*file_size);

    if (role == "producer") {

      std::cerr << "Begin warmups" << '\n';

      for(int i = 0; i < warmup_reps; ++i) {

        std::string new_key = key + "_" + std::to_string(i);
        std::string new_key_response = key + "_" + std::to_string(i) + "_response";
        upload_random_file(c, new_key, file_size, pBuf);
        auto download_ = download_file(c, new_key_response, report_dl_time);
        if(download_ == 0) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }
        delete_file(c, new_key);
        delete_file(c, new_key_response);
      } 
      std::cerr << "Begin normal" << '\n';

      for(int i = warmup_reps; i < reps + warmup_reps; ++i) {

        std::string new_key = key + "_" + std::to_string(i);
        std::string new_key_response = key + "_" + std::to_string(i) + "_response";
        auto beg = timeSinceEpochMillisec();
        upload_random_file(c, new_key, file_size, pBuf);
        auto download_ = download_file(c, new_key_response, report_dl_time);
        if(download_ == 0) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }
        auto end = timeSinceEpochMillisec();
        times.push_back(end - beg);
        delete_file(c, new_key);
        delete_file(c, new_key_response);
      } 

      std::stringstream ss;
      ss << times.size() << '\n';
      for(int i = 0; i < times.size(); ++i)
        ss << times[i] << '\n';

      auto times_str = ss.str();
      char* data = new char[times_str.length()];
      strcpy(data, times_str.c_str());

      std::string new_key = key + "_" + std::to_string(file_size)+ "_producer_times";
      upload_random_file_s3(client, bucket, new_key, times_str.length(), data);

      delete[] data;
    } else if (role == "consumer") {

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      std::cerr << "Begin" << '\n';

      for(int i = 0; i < warmup_reps; ++i) {

        std::string new_key = key + "_" + std::to_string(i);
        std::string new_key_response = key + "_" + std::to_string(i) + "_response";
        auto download_ = download_file(c, new_key, report_dl_time);
        if(download_ == 0) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }
        upload_random_file(c, new_key_response, file_size, pBuf);
      } 
      std::cerr << "Begin normal" << '\n';

      for(int i = warmup_reps; i < reps + warmup_reps; ++i) {

        std::string new_key = key + "_" + std::to_string(i);
        std::string new_key_response = key + "_" + std::to_string(i) + "_response";
        auto download_ = download_file(c, new_key, report_dl_time);
        if(download_ == 0) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }
        upload_random_file(c, new_key_response, file_size, pBuf);

      } 
    }

    delete[] pBuf;
    std::string res_json = "{ \"fileSize\": " + std::to_string(file_size) + "}";
    
    redisFree(c);
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
        auto handler_fn = [&client](aws::lambda_runtime::invocation_request const &req)
        {
            return my_handler(req, client);
        };
        run_handler(handler_fn);
    }
    ShutdownAPI(options);
    return 0;
}

uint64_t download_file(redisContext* context,
                       std::string const &key,
                       bool report_dl_time)
{
    int retries = 0;
    const int MAX_RETRIES = 50000;
    auto bef = timeSinceEpochMillisec();
    while (retries < MAX_RETRIES) {
        std::string comm = "GET " + key;
        redisReply* reply = (redisReply*) redisCommand(context, comm.c_str());
        if (reply->type == REDIS_REPLY_NIL || reply->type == REDIS_REPLY_ERROR) {
            retries += 1;
            freeReplyObject(reply);
        } else {
            uint64_t finishedTime = timeSinceEpochMillisec();
            // Perform NOP on result to prevent optimizations
            for (int i = 0; i < reply->elements; i++) {
                std::string res = reply->element[i]->str;
            }
            freeReplyObject(reply);
            if (report_dl_time) {
                return finishedTime - bef;
            } else {
                return finishedTime;
            }
            
        }
    }
    return 0;

}

uint64_t upload_random_file(redisContext* context,
                        std::string const &key,
                        int size, char *pBuf)
{

    uint64_t bef_upload = timeSinceEpochMillisec();
    std::string comm = "SET " + key + " %b";
    redisReply* reply = (redisReply*) redisCommand(context, comm.c_str(), pBuf, size);
    freeReplyObject(reply);
    //delete[] pBuf;
    return bef_upload;
}

uint64_t delete_file(redisContext* context,
                        std::string const &key)
{

    uint64_t bef_upload = timeSinceEpochMillisec();
    std::string comm = "DEL " + key;
    redisReply* reply = (redisReply*) redisCommand(context, comm.c_str());
    if (reply->type == REDIS_REPLY_NIL || reply->type == REDIS_REPLY_ERROR) {
      std::cerr << "Couldn't delete the key!" << '\n';
      abort();
    }
    freeReplyObject(reply);
    //delete[] pBuf;
    return bef_upload;
}
