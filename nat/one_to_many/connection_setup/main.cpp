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

void send_file(int sock_fd,
                   const struct sockaddr_in &peeraddr);

void send_file_multiple(int sock_fds[],
                   struct sockaddr_in peeraddr[],
                   int num_consumers);

void get_file(int sock_fd,
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
    if (v.KeyExists("waitUntil")) {
        auto wait_until = v.GetInteger("waitUntil");
        while (std::time(0) < wait_until) {
            
        }

    }
    int num_consumers = 1;
    if (v.KeyExists("numConsumers")) {
        num_consumers = v.GetInteger("numConsumers");
    }
    std::cout << "Invoked handler for role " << role << std::endl;

        

    std::string res_json;
    if (role == "producer")
    {
        uint64_t bef_setup = timeSinceEpochMillisec();
        struct sockaddr_in* peeraddrs = new struct sockaddr_in[num_consumers];
        int* socket_fds = new int[num_consumers];
        for (int i = 0; i < num_consumers; i++) {
            std::string prod_pairing_key = pairing_key + "_" + std::to_string(i + 1);
            socket_fds[i] = pair(peeraddrs[i], prod_pairing_key, server_ip);
        }
        
        if (num_consumers == 1) {
            send_file(socket_fds[0], peeraddrs[0]);
        } else {
            send_file_multiple(socket_fds, peeraddrs, num_consumers);
        }
        uint64_t after_setup = timeSinceEpochMillisec();
        res_json = "{\"setupTime\":" + std::to_string(after_setup - bef_setup) + "}";
        delete[] peeraddrs;
        delete[] socket_fds;
    }
    else if (role == "consumer")
    {
        struct sockaddr_in peeraddr;
        int socket_fd = pair(peeraddr, pairing_key, server_ip);
        get_file(socket_fd, peeraddr);
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

void get_file(int sock_fd,
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

    UDT::close(u_sock);
    UDT::cleanup();
}

void send_file(int sock_fd,
                   const struct sockaddr_in &peeraddr)
{         
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
}

void send_file_multiple(int sock_fds[],
                   struct sockaddr_in peeraddr[],
                   int num_consumers)
{
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
 
    for (int i = 0; i < num_consumers; i++) {
        UDT::close(u_socks[i]);
    }
    
    
    

    UDT::cleanup();

    delete[] u_socks;    
}