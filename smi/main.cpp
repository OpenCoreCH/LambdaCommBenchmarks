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

unsigned long get_time_in_microseconds() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return 1000000 * tv.tv_sec + tv.tv_usec;
}

void bcast_benchmark(SMI::Comm::Data<int>& data, SMI::Communicator& comm) {
    comm.bcast(data, 0);
}

void gather_benchmark(SMI::Comm::Data<std::vector<int>>& sendbuf, SMI::Comm::Data<std::vector<int>>& recvbuf, SMI::Communicator& comm) {
    comm.gather(sendbuf, recvbuf, 0);
}

void scatter_benchmark(SMI::Comm::Data<std::vector<int>>& sendbuf, SMI::Comm::Data<std::vector<int>>& recvbuf, SMI::Communicator& comm) {
    comm.scatter(sendbuf, recvbuf, 0);
}

void reduce_benchmark(SMI::Comm::Data<int>& data, SMI::Communicator& comm) {
    SMI::Comm::Data<int> res;
    SMI::Utils::Function<int> f([] (auto a, auto b) {return a + b;}, true, true);
    comm.reduce(data, res, 0, f);
}

void allreduce_benchmark(SMI::Comm::Data<int>& data, SMI::Communicator& comm) {
    SMI::Comm::Data<int> res;
    SMI::Utils::Function<int> f([] (auto a, auto b) {return a + b;}, true, true);
    comm.allreduce(data, res, f);
}

void scan_benchmark(SMI::Comm::Data<int>& data, SMI::Communicator& comm) {
    SMI::Comm::Data<int> res;
    SMI::Utils::Function<int> f([] (auto a, auto b) {return a + b;}, true, true);
    comm.scan(data, res, f);
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
    std::string benchmark = v.GetString("benchmark");
    std::string timestamp = v.GetString("timestamp");

    SMI::Communicator comm(peer_id, num_peers, "smi.json", timestamp, 512);


    std::string res;

    for (int i = 0; i < 30; i++) {
        unsigned long bef, after;
        comm.barrier();
        if (benchmark == "bcast") {
            SMI::Comm::Data<int> data = peer_id + 1;
            bef = get_time_in_microseconds();
            bcast_benchmark(data, comm);
            after = get_time_in_microseconds();
        } else if (benchmark == "gather") {
            long recv_size = 4992;
            long individual_size = recv_size / num_peers;
            SMI::Comm::Data<std::vector<int>> recv(recv_size);
            SMI::Comm::Data<std::vector<int>> send(individual_size);
            bef = get_time_in_microseconds();
            gather_benchmark(send, recv, comm);
            after = get_time_in_microseconds();
        } else if (benchmark == "scatter") {
            long send_size = 4992;
            long individual_size = send_size / num_peers;
            SMI::Comm::Data<std::vector<int>> recv(individual_size);
            SMI::Comm::Data<std::vector<int>> send(send_size);
            bef = get_time_in_microseconds();
            scatter_benchmark(send, recv, comm);
            after = get_time_in_microseconds();
        } else if (benchmark == "reduce") {
            SMI::Comm::Data<int> data = peer_id + 1;
            bef = get_time_in_microseconds();
            reduce_benchmark(data, comm);
            after = get_time_in_microseconds();
        } else if (benchmark == "allreduce") {
            SMI::Comm::Data<int> data = peer_id + 1;
            bef = get_time_in_microseconds();
            allreduce_benchmark(data, comm);
            after = get_time_in_microseconds();
        } else if (benchmark == "scan") {
            SMI::Comm::Data<int> data = peer_id + 1;
            bef = get_time_in_microseconds();
            scan_benchmark(data, comm);
            after = get_time_in_microseconds();
        }
        if (peer_id == 0) {
            if (benchmark == "gather") {
                res.append(std::to_string(peer_id) + "," + std::to_string(i) + "," + std::to_string(after) + '\n');
            } else {
                res.append(std::to_string(peer_id) + "," + std::to_string(i) + "," + std::to_string(bef) + '\n');
            }
        } else {
            if (benchmark == "gather") {
                res.append(std::to_string(peer_id) + "," + std::to_string(i) + "," + std::to_string(bef) + '\n');
            } else {
                res.append(std::to_string(peer_id) + "," + std::to_string(i) + "," + std::to_string(after) + '\n');
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(20000));
        
        
    }
    
    return invocation_response::success(res, "application/txt");
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
