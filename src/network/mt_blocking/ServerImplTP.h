#ifndef AFINA_NETWORK_MT_BLOCKING_SERVER_THREAD_POOL_H
#define AFINA_NETWORK_MT_BLOCKING_SERVER_THREAD_POOL_H

#include <atomic>
#include <thread>
#include <set>
#include <condition_variable>

#include "afina/network/Server.h"
#include "afina/concurrency/Executor.h"

namespace spdlog {
class logger;
}

namespace Afina {
namespace Network {
namespace MTblocking {

class ServerImplTP;

void ConnectionWorker(ServerImplTP *server, int client_socket);
void AcceptWorker(ServerImplTP *server, int server_socket);

class ServerImplTP: public Server {
public:
    ServerImplTP(std::shared_ptr<Afina::Storage> ps, std::shared_ptr<Logging::Service> pl,
        size_t low_watermark = 0, size_t hight_watermark = 16, size_t max_queue_size = 128, std::chrono::milliseconds idle_time = std::chrono::milliseconds{5000});
    ~ServerImplTP();

    // See Server.h
    void Start(uint16_t port, uint32_t, uint32_t) override;

    // See Server.h
    void Stop() override;

    // See Server.h
    void Join() override;

    friend void ConnectionWorker(ServerImplTP *server, int client_socket);
    friend void AcceptWorker(ServerImplTP *server, int server_socket);

private:
    // Logger instance
    std::shared_ptr<spdlog::logger> _logger;

    std::atomic<bool> running;

    Concurrency::Executor thread_pool;

    std::mutex _mutex;
    std::set<int> _sockets;
};

} // namespace MTblocking
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_BLOCKING_SERVER_H
