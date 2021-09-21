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
#include <hole_punching_client.h>
#include <udt.h>

using namespace aws::lambda_runtime;
char const TAG[] = "LAMBDA_ALLOC";

uint64_t send_file(int sock_fd,
                   const struct sockaddr_in &peeraddr,
                   int size);

uint64_t send_file_multiple(int sock_fds[],
                   struct sockaddr_in peeraddr[],
                   int num_consumers,
                   int size);

uint64_t get_file(int sock_fd,
                  const struct sockaddr_in &peeraddr);

uint64_t timeSinceEpochMillisec()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
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

        

    std::string res_json = "{ \"fileSize\": " + std::to_string(file_size) + ", \"role\": \"" + role + "\"" ;
    if (role == "producer")
    {
        struct sockaddr_in* peeraddrs = new struct sockaddr_in[num_consumers];
        int* socket_fds = new int[num_consumers];
        for (int i = 0; i < num_consumers; i++) {
            std::string prod_pairing_key = pairing_key + "_" + std::to_string(i + 1);
            socket_fds[i] = pair(peeraddrs[i], prod_pairing_key, server_ip);
        }
        uint64_t upload_time;
        if (num_consumers == 1) {
            upload_time = send_file(socket_fds[0], peeraddrs[0], file_size);
        } else {
            upload_time = send_file_multiple(socket_fds, peeraddrs, num_consumers, file_size);
        }
        res_json += ", \"uploadTime\": " + std::to_string(upload_time) + " }";
        delete[] peeraddrs;
        delete[] socket_fds;
    }
    else if (role == "consumer")
    {
        struct sockaddr_in peeraddr;
        int socket_fd = pair(peeraddr, pairing_key, server_ip);
        uint64_t finished_time = get_file(socket_fd, peeraddr);
        res_json += ", \"finishedTime\": " + std::to_string(finished_time) + " }";
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

uint64_t get_file(int sock_fd,
                  const struct sockaddr_in &peeraddr)
{
    int size;

    UDT::startup();
    UDTSOCKET u_sock = UDT::socket(AF_INET, SOCK_STREAM, 0);
    bool rdv = true;
    struct addrinfo *u_peer;
    UDT::setsockopt(u_sock, 0, UDT_RENDEZVOUS, &rdv, sizeof(bool));
    if (UDT::ERROR == UDT::bind2(u_sock, sock_fd)) {
        std::cout << "bind2: " << UDT::getlasterror().getErrorMessage() << std::endl;
    }
    if (UDT::ERROR == UDT::connect(u_sock, (const struct sockaddr *) &peeraddr, sizeof(peeraddr))) {
        std::cout << "connect: " << UDT::getlasterror().getErrorMessage() << std::endl;
    }
    std::cout << "Succesfully connected" << std::endl;
    if (UDT::ERROR == UDT::recv(u_sock, (char*) (size_t) &size, sizeof(int), 0)) {
        std::cout << "recv: " << UDT::getlasterror().getErrorMessage() << std::endl;
    }
    std::cout << "Size is " << size << std::endl;
    char* recv_buffer = new char[size];
    int total_recv_bytes = 0;
    while (total_recv_bytes < size) {
        int recv_bytes = UDT::recv(u_sock, recv_buffer + total_recv_bytes, size - total_recv_bytes, 0);
        if (recv_bytes == UDT::ERROR) {
            std::cout << "recv: " << UDT::getlasterror().getErrorMessage() << std::endl;
        } else {
            std::cout << "Received " << recv_bytes << " bytes" << std::endl;
        }
        total_recv_bytes += recv_bytes;
    }



    UDT::close(u_sock);
    UDT::cleanup();

    delete[] recv_buffer;
    return timeSinceEpochMillisec();
}

uint64_t send_file(int sock_fd,
                   const struct sockaddr_in &peeraddr,
                   int size)
{      
    char* pBuf = new char[size];
    
    
    UDT::startup();
    UDTSOCKET u_sock = UDT::socket(AF_INET, SOCK_STREAM, 0);
    bool rdv = true;
    UDT::setsockopt(u_sock, 0, UDT_RENDEZVOUS, &rdv, sizeof(bool));
    if (UDT::ERROR == UDT::bind2(u_sock, sock_fd)) {
        std::cout << "bind2: " << UDT::getlasterror().getErrorMessage() << std::endl;
    }
    if (UDT::ERROR == UDT::connect(u_sock, (const struct sockaddr *) &peeraddr, sizeof(peeraddr))) {
        std::cout << "connect: " << UDT::getlasterror().getErrorMessage() << std::endl;
    }
    std::cout << "Succesfully connected" << std::endl;
    uint64_t bef_upload = timeSinceEpochMillisec();
    if (UDT::ERROR == UDT::send(u_sock, (char*) (size_t) &size, sizeof(int), 0)) {
        std::cout << "send: " << UDT::getlasterror().getErrorMessage() << std::endl;
    }
    int total_sent_bytes = 0;
    while (total_sent_bytes < size) {
        int sent_bytes = UDT::send(u_sock, pBuf + total_sent_bytes, size - total_sent_bytes, 0);
        if (sent_bytes == UDT::ERROR) {
            std::cout << "send: " << UDT::getlasterror().getErrorMessage() << std::endl;
        }
        total_sent_bytes += sent_bytes;
    }
    
    
    UDT::close(u_sock);
    UDT::cleanup();

    delete[] pBuf;
    
    
    return bef_upload;
}

uint64_t send_file_multiple(int sock_fds[],
                   struct sockaddr_in peeraddr[],
                   int num_consumers,
                   int size)
{
    char* pBuf = new char[size];
    UDT::startup();
    UDTSOCKET* u_socks = new UDTSOCKET[num_consumers];
    for (int i = 0; i < num_consumers; i++) {
        u_socks[i] = UDT::socket(AF_INET, SOCK_STREAM, 0);
        bool rdv = true;
        UDT::setsockopt(u_socks[i], 0, UDT_RENDEZVOUS, &rdv, sizeof(bool));
        if (UDT::ERROR == UDT::bind2(u_socks[i], sock_fds[i])) {
            std::cout << "bind2: " << UDT::getlasterror().getErrorMessage() << std::endl;
        }
        if (UDT::ERROR == UDT::connect(u_socks[i], (const struct sockaddr *) &peeraddr[i], sizeof(peeraddr[i]))) {
            std::cout << "connect: " << UDT::getlasterror().getErrorMessage() << std::endl;
        }
    }
    std::cout << "Succesfully connected" << std::endl;
    uint64_t bef_upload = timeSinceEpochMillisec();
    for (int i = 0; i < num_consumers; i++) {

    }
    for (int i = 0; i < num_consumers; i++) {
        if (UDT::ERROR == UDT::send(u_socks[i], (char*) (size_t) &size, sizeof(int), 0)) {
            std::cout << "send: " << UDT::getlasterror().getErrorMessage() << std::endl;
        }
    }

    int* total_sent_bytes = new int[num_consumers]();
    int curr_consumer = 0;
    bool finished = false;
    while (!finished) {
        if (total_sent_bytes[curr_consumer] < size) {
            int sent_bytes = UDT::send(u_socks[curr_consumer], pBuf + total_sent_bytes[curr_consumer], size - total_sent_bytes[curr_consumer], 0);
            if (sent_bytes == UDT::ERROR) {
                std::cout << "send: " << UDT::getlasterror().getErrorMessage() << std::endl;
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
    for (int i = 0; i < num_consumers; i++) {
        UDT::close(u_socks[i]);
    }
    
    
    

    UDT::cleanup();

    delete[] pBuf;
    delete[] u_socks;
    
    
    return bef_upload;
}