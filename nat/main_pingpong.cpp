#include <aws/core/Aws.h>
#include <aws/core/utils/logging/LogLevel.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/core/client/ClientConfiguration.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/lambda-runtime/runtime.h>
#include <boost/interprocess/streams/bufferstream.hpp>
#include <aws/lambda-runtime/runtime.h>
#include <vector>
#include <random>
#include <chrono>
#include <thread>
#include <cstdint>
#include <iostream>
#include <tcpunch.h>
#include <fcntl.h>

using namespace aws::lambda_runtime;
char const TAG[] = "LAMBDA_ALLOC";

uint64_t send_file(int sock_fd,
                   int size,char*);

uint64_t send_file_multiple(int sock_fds[],
                   int num_consumers,
                   int size);

uint64_t get_file(int sock_fd,
                  int size,char*);

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

static invocation_response my_handler(invocation_request const &req, Aws::S3::S3Client const &client)
{
    using namespace Aws::Utils::Json;
    JsonValue json(req.payload);
    if (!json.WasParseSuccessful())
    {
        return invocation_response::failure("Failed to parse input JSON", "InvalidJSON");
    }

    auto v = json.View();

    auto pairing_key = v.GetString("key");
    auto server_ip = v.GetString("ip");
    auto role = v.GetString("role"); // producer or consumer
    auto file_size = v.GetInteger("fileSize");
    if (v.KeyExists("waitUntil")) {
        auto wait_until = v.GetInteger("waitUntil");
        while (std::time(0) < wait_until) {
            
        }

    }
    int num_consumers = 1;
    if (v.KeyExists("numConsumers")) {
        num_consumers = v.GetInteger("numConsumers");
    }
    std::cout << "Invoked handler for role " << role << " with file size " << file_size << std::endl;
    std::string bucket = "s3benchmark-eu";

    std::vector<uint64_t> times;
    int warmup_reps = 5;
    //int reps = 1000;
    int reps = 10;
    //int warmup_reps = 1;
    //int reps = 2;
    int retries = 0;

    char* recv_buffer = new char[file_size];
    char* pBuf = new char[file_size];
    memset(pBuf, 1, sizeof(char)*file_size);


    if (role == "producer") {

      int* socket_fds = new int[num_consumers];
      for (int i = 0; i < num_consumers; i++) {
          std::string prod_pairing_key = pairing_key + "_" + std::to_string(i + 1);
          std::cerr << "Begin pairing " << prod_pairing_key << '\n';
          socket_fds[i] = pair(prod_pairing_key, server_ip);
      }

      std::cerr << "Begin warmups" << '\n';

      for(int i = 0; i < warmup_reps; ++i) {

        auto send_ = send_file(socket_fds[0], file_size, pBuf);
        if(send_ == 0) {
          std::cerr << "Failed send " << i << '\n';
          break;
        }
        auto download_ = get_file(socket_fds[0], file_size, recv_buffer);
        if(download_ == 1) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }
      } 
      std::cerr << "Begin normal" << '\n';

      for(int i = warmup_reps; i < reps + warmup_reps; ++i) {

        auto beg = timeSinceEpochMillisec();
        auto send_ = send_file(socket_fds[0], file_size,pBuf);
        if(send_ == 0) {
          std::cerr << "Failed send " << i << '\n';
          break;
        }
        auto download_ = get_file(socket_fds[0], file_size, recv_buffer);
        if(download_ == 1) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }
        auto end = timeSinceEpochMillisec();
        times.push_back(end - beg);
      } 

      std::stringstream ss;
      ss << times.size() << '\n';
      for(int i = 0; i < times.size(); ++i)
        ss << times[i] << '\n';

      auto times_str = ss.str();
      char* data = new char[times_str.length()];
      strcpy(data, times_str.c_str());

      std::string new_key = pairing_key + "_" + std::to_string(file_size)+ "_producer_times";
      upload_random_file_s3(client, bucket, new_key, times_str.length(), data);

      delete[] data;
      delete[] socket_fds;
    } else if (role == "consumer") {

      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      std::cerr << "Begin pairing " << pairing_key << '\n';
      int socket_fd = pair(pairing_key, server_ip);
      std::cerr << "Begin" << '\n';

      for(int i = 0; i < warmup_reps; ++i) {

        auto download_ = get_file(socket_fd, file_size, recv_buffer);
        if(download_ == 1) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }
        auto send_ = send_file(socket_fd, file_size, pBuf);
        if(send_ == 0) {
          std::cerr << "Failed send " << i << '\n';
          break;
        }
      } 
      std::cerr << "Begin normal" << '\n';

      for(int i = warmup_reps; i < reps + warmup_reps; ++i) {

        auto download_ = get_file(socket_fd, file_size, recv_buffer);
        if(download_ == 1) {
          std::cerr << "Failed download " << i << '\n';
          break;
        }
        auto send_ = send_file(socket_fd, file_size, pBuf);
        if(send_ == 0) {
          std::cerr << "Failed send " << i << '\n';
          break;
        }

      } 
    }

    delete[] recv_buffer;
    delete[] pBuf;
    std::string res_json = "{ \"fileSize\": " + std::to_string(file_size) + "}";
    
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

uint64_t get_file(int sock_fd,
                  int size, char* recv_buffer)
{
    uint64_t download_time = 0;
    
    int recv_bytes = 0;
    while (recv_bytes < size) {
        int n = recv(sock_fd, recv_buffer + recv_bytes, size - recv_bytes, 0);
        if (n > 0) {
            recv_bytes += n;
        }
        if (n == -1) {
            std::cout << "Error: " << errno << std::endl;
            download_time = 1;
            break;
        }
    }

    if (download_time != 1) {
        download_time = timeSinceEpochMillisec();
    }

    return download_time;
}

uint64_t send_file(int sock_fd,
                   int size, char* pBuf)
{      

    uint64_t bef_upload = timeSinceEpochMillisec();

    int sent_bytes = 0;
    while (sent_bytes < size) {
        int bytes = send(sock_fd, pBuf + sent_bytes, size - sent_bytes, 0);
        sent_bytes += bytes;
        if (bytes == -1) {
            bef_upload = 0;
            break;
        }
    }
    return bef_upload;
}

uint64_t send_file_multiple(int sock_fds[],
                   int num_consumers,
                   int size)
{
    char* pBuf = new char[size];

    uint64_t bef_upload = timeSinceEpochMillisec();

    int* total_sent_bytes = new int[num_consumers]();
    int curr_consumer = 0;
    bool finished = false;
    while (!finished) {
        if (total_sent_bytes[curr_consumer] < size) {
            int sent_bytes = send(sock_fds[curr_consumer], pBuf + total_sent_bytes[curr_consumer], size - total_sent_bytes[curr_consumer], 0);
            if (sent_bytes == -1) {
                std::cout << "send: " << errno << std::endl;
                bef_upload = 0;
                break;
            }
            total_sent_bytes[curr_consumer] += sent_bytes;
        }
        curr_consumer = (curr_consumer + 1) % num_consumers;
        
        // Check if all done every round
        if (curr_consumer == 0) {
            finished = true;
            for (int i = 0; i < num_consumers; i++) {
                if (total_sent_bytes[i] < size) {
                    finished = false;
                }
            }
        }
    }

    delete[] pBuf;   
    
    return bef_upload;
}
