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
    std::cout << "Invoked handler for role " << role << " with file size " << file_size << std::endl;

    struct sockaddr_in peeraddr;
    int socket_fd = pair(peeraddr, pairing_key, server_ip);
    sockaddr_in name4;
    sockaddr_in6 name6;
    sockaddr* name;
    socklen_t namelen;
    namelen = sizeof(sockaddr_in);
    name = (sockaddr*)&name4;
        

    std::string res_json = "{ \"fileSize\": " + std::to_string(file_size) + ", \"role\": \"" + role + "\"" ;
    if (role == "producer")
    {
        uint64_t upload_time = send_file(socket_fd, peeraddr, file_size);
        res_json += ", \"uploadTime\": " + std::to_string(upload_time) + " }";
    }
    else if (role == "consumer")
    {
        //sendto(socket_fd, "1", 1, MSG_CONFIRM, (const struct sockaddr *) &peeraddr, sizeof(peeraddr));
        
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
    if (UDT::ERROR == UDT::recv(u_sock, recv_buffer, size, 0)) {
        std::cout << "recv: " << UDT::getlasterror().getErrorMessage() << std::endl;
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
    struct addrinfo *u_peer;
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
    if (UDT::ERROR == UDT::send(u_sock, pBuf, size, 0)) {
        std::cout << "send: " << UDT::getlasterror().getErrorMessage() << std::endl;
    }
    
    UDT::close(u_sock);
    UDT::cleanup();

    delete[] pBuf;
    
    
    return bef_upload;
}