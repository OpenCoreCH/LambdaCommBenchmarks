#include <iostream>
#include <sys/time.h>
#include <Communicator.h>
#include <cstdlib>


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

int main(int argc, char** argv) {
    std::string benchmark(argv[1]);
    int num_peers = std::stoi(std::getenv("OMPI_COMM_WORLD_SIZE"));
    int peer_id = std::stoi(std::getenv("OMPI_COMM_WORLD_RANK"));

    SMI::Communicator comm(peer_id, num_peers, "smi.json", "SMITest", 512);
    

    for (int i = 0; i < 1001; i++) {
        unsigned long bef, after;
        comm.barrier();
        if (benchmark == "bcast") {
            SMI::Comm::Data<int> data = peer_id + 1;
            bef = get_time_in_microseconds();
            bcast_benchmark(data, comm);
            after = get_time_in_microseconds();
        } else if (benchmark == "gather") {
            long recv_size = 5000;
            long individual_size = recv_size / num_peers;
            SMI::Comm::Data<std::vector<int>> recv(recv_size);
            SMI::Comm::Data<std::vector<int>> send(individual_size);
            bef = get_time_in_microseconds();
            gather_benchmark(send, recv, comm);
            after = get_time_in_microseconds();
        } else if (benchmark == "scatter") {
            long send_size = 5000;
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
        if (i > 0) {
            if (peer_id == 0) {
                if (benchmark == "gather") {
                    std::cout << peer_id << "," << i << "," << after << std::endl;
                } else {
                    std::cout << peer_id << "," << i << "," << bef << std::endl;
                }
            } else {
                if (benchmark == "gather") {
                    std::cout << peer_id << "," << i << "," << bef << std::endl;
                } else {
                    std::cout << peer_id << "," << i << "," << after << std::endl;
                }
            }
        }
        
    }
}


