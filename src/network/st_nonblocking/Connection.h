#ifndef AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
#define AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H

#include <cstring>
#include <string>
#include <deque>
#include <memory>

#include <spdlog/logger.h>
#include <afina/Storage.h>
#include "protocol/Parser.h"
#include <afina/execute/Command.h>

#include <sys/epoll.h>

namespace Afina {
namespace Network {
namespace STnonblock {

class Connection {
public:
    Connection(int s) : _socket(s), _buff_offset(0), _head_offset(0), _alive(true) {
        std::memset(&_event, 0, sizeof(struct epoll_event));
        _event.data.ptr = this;
    }

    inline bool isAlive() const { return _alive; }

    void Start(std::shared_ptr<spdlog::logger> _logger);

protected:
    void OnError(std::shared_ptr<spdlog::logger> _logger);
    void OnClose(std::shared_ptr<spdlog::logger> _logger);
    void DoRead(std::shared_ptr<Afina::Storage> pStorage, std::shared_ptr<spdlog::logger> _logger);
    void DoWrite(std::shared_ptr<spdlog::logger> _logger);

private:
    friend class ServerImpl;

    int _socket;
    struct epoll_event _event;

    size_t _buff_offset;
    char _client_buffer[4096];

    size_t _head_offset;
    std::deque<std::string> _outgoing;

    bool _alive;

    std::size_t arg_remains;
    Protocol::Parser parser;
    std::string argument_for_command;
    std::unique_ptr<Execute::Command> command_to_execute;
};

} // namespace STnonblock
} // namespace Network
} // namespace Afina

#endif // AFINA_NETWORK_ST_NONBLOCKING_CONNECTION_H
