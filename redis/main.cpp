#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/lambda-runtime/runtime.h>
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
                        int size);

uint64_t download_file(redisContext* context,
                       std::string const &key,
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
    redisContext *c = redisConnect(redis_hostname.c_str(), redis_port);
    if (c == NULL || c->err) {
        if (c) {
            printf("Redis Error: %s\n", c->errstr);
        } else {
            printf("Can't allocate redis context\n");
        }
    }


    std::string res_json = "{ \"fileSize\": " + std::to_string(file_size) + ", \"role\": \"" + role + "\"" ;
    if (role == "producer")
    {
        //redisCommand(c, "FLUSHALL");
        uint64_t upload_time = upload_random_file(c, key, file_size);
        res_json += ", \"uploadTime\": " + std::to_string(upload_time) + " }";
    }
    else if (role == "consumer")
    {
        uint64_t finished_time = download_file(c, key, report_dl_time);
        res_json += ", \"finishedTime\": " + std::to_string(finished_time) + " }";
    }
    
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
                        int size)
{
    char* pBuf = new char[size];

    uint64_t bef_upload = timeSinceEpochMillisec();
    std::string comm = "SET " + key + " %b";
    redisReply* reply = (redisReply*) redisCommand(context, comm.c_str(), pBuf, size);
    freeReplyObject(reply);
    delete[] pBuf;
    return bef_upload;
}