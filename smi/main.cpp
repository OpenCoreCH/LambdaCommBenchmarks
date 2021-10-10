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
#include <tcpunch.h>
#include <fcntl.h>
#include <Communicator.h>

using namespace aws::lambda_runtime;
char const TAG[] = "LAMBDA_ALLOC";

uint64_t timeSinceEpochMicrosec()
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

    int num_peers = v.GetInteger("numPeers");
    int peer_id = v.GetInteger("peerID");

    SMI::Communicator comm(peer_id, num_peers, "smi.json", "SMITest", 512);

    SMI::Comm::Data<int> data = peer_id + 1;
    SMI::Comm::Data<int> res;

    SMI::Utils::Function<int> f([] (auto a, auto b) {return a + b;}, true, true);

    std::string res_json = "{ \"peerID\": " + std::to_string(peer_id) + ", \"numPeers\": \"" + std::to_string(num_peers) + "\"" ;

    auto bef = timeSinceEpochMicrosec();
    comm.reduce(data, res, 0, f);
    auto after = timeSinceEpochMicrosec();
    res_json += ", \"time\": " + std::to_string(after - bef) + " }";
    
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