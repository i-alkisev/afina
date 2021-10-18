#ifndef AFINA_NETWORK_MT_BLOCKING_SERVER_H
#define AFINA_NETWORK_MT_BLOCKING_SERVER_H

#include <atomic>
#include <thread>
#include <set>
#include <condition_variable>

#include "afina/network/Server.h"

namespace spdlog {
class logger;
}

namespace Afina {
namespace Network {
namespace MTblocking {

/**
 * # Network resource manager implementation
 * Server that is spawning a separate thread for each connection
 */
class ServerImpl : public Server {
public:
    ServerImpl(std::shared_ptr<Afina::Storage> ps, std::shared_ptr<Logging::Service> pl, std::size_t max_count = 16);
    ~ServerImpl();

    // See Server.h
    void Start(uint16_t port, uint32_t, uint32_t) override;

    // See Server.h
    void Stop() override;

    // See Server.h
    void Join() override;

protected:
    /**
     * Method is running in the connection acceptor thread
     */
    void OnRun(int server_socket);
    void WorkerThread(int client_socket);

    // Implementation with ThreadPool
    void OnRunImpl2(int server_socket);

private:
    // Logger instance
    std::shared_ptr<spdlog::logger> _logger;

    // Atomic flag to notify threads when it is time to stop. Note that
    // flag must be atomic in order to safely publisj changes cross thread
    // bounds
    std::atomic<bool> running;

    // Server socket to accept connections on
    // int _server_socket;

    // Thread to run network on
    std::thread _thread;

    std::mutex _mutex;
    std::size_t _max_connections;

    std::set<int> _sockets;

    std::condition_variable _final;
};

// Implementation for perform on the ThreadPool
void WorkerThreadImpl2(std::shared_ptr<Afina::Storage> pStorage, std::shared_ptr<spdlog::logger> _logger, int client_socket);

} // namespace MTblocking
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_MT_BLOCKING_SERVER_H
