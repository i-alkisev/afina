#include "Connection.h"

#include <iostream>

namespace Afina {
namespace Network {
namespace STnonblock {

// See Connection.h
void Connection::Start(std::shared_ptr<spdlog::logger> _logger) {
    _logger->debug("New connection");
    _event.events |= EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    }

// See Connection.h
void Connection::OnError(std::shared_ptr<spdlog::logger> _logger) {
    _logger->error("Failed to process connection on descriptor {}", _socket);
    _event.events &= 0;
    _alive = false;
}

// See Connection.h
void Connection::OnClose(std::shared_ptr<spdlog::logger> _logger) {
    _logger->debug("Close connection");
    _event.events &= 0;
    _alive = false;
    }

// See Connection.h
void Connection::DoRead(std::shared_ptr<Afina::Storage> pStorage, std::shared_ptr<spdlog::logger> _logger) {
    _logger->debug("Reading");
    try {
        int readed_bytes = -1;
        while ((readed_bytes = read(_socket, _client_buffer + _buff_offset, sizeof(_client_buffer) - _buff_offset)) > 0) {
            _buff_offset += readed_bytes;
            _logger->debug("Got {} bytes from socket", readed_bytes);

            // Single block of data readed from the socket could trigger inside actions a multiple times,
            // for example:
            // - read#0: [<command1 start>]
            // - read#1: [<command1 end> <argument> <command2> <argument for command 2> <command3> ... ]
            readed_bytes = _buff_offset;
            while (readed_bytes > 0) {
                _logger->debug("Process {} bytes", readed_bytes);
                // There is no command yet
                if (!command_to_execute) {
                    std::size_t parsed = 0;
                    if (parser.Parse(_client_buffer, readed_bytes, parsed)) {
                        // There is no command to be launched, continue to parse input stream
                        // Here we are, current chunk finished some command, process it
                        _logger->debug("Found new command: {} in {} bytes", parser.Name(), parsed);
                        command_to_execute = parser.Build(arg_remains);
                        if (arg_remains > 0) {
                            arg_remains += 2;
                        }
                    }

                    // Parsed might fails to consume any bytes from input stream. In real life that could happens,
                    // for example, because we are working with UTF-16 chars and only 1 byte left in stream
                    if (parsed == 0) {
                        break;
                    } else {
                        std::memmove(_client_buffer, _client_buffer + parsed, readed_bytes - parsed);
                        readed_bytes -= parsed;
                        _buff_offset = readed_bytes;
                    }
                }

                // There is command, but we still wait for argument to arrive...
                if (command_to_execute && arg_remains > 0) {
                    _logger->debug("Fill argument: {} bytes of {}", readed_bytes, arg_remains);
                    // There is some parsed command, and now we are reading argument
                    std::size_t to_read = std::min(arg_remains, std::size_t(readed_bytes));
                    argument_for_command.append(_client_buffer, to_read);

                    std::memmove(_client_buffer, _client_buffer + to_read, readed_bytes - to_read);
                    arg_remains -= to_read;
                    readed_bytes -= to_read;
                    _buff_offset = readed_bytes > 0 ? readed_bytes : 0;
                }

                // Thre is command & argument - RUN!
                if (command_to_execute && arg_remains == 0) {
                    _logger->debug("Start command execution");

                    std::string result;
                    if (argument_for_command.size()) {
                        argument_for_command.resize(argument_for_command.size() - 2);
                    }
                    command_to_execute->Execute(*pStorage, argument_for_command, result);

                    // Send response
                    result += "\r\n";

                    _outgoing.push_back(result);

                    _event.events |= EPOLLOUT;

                    // Prepare for the next command
                    command_to_execute.reset();
                    argument_for_command.resize(0);
                    parser.Reset();

                    return;
                }
            } // while (readed_bytes)
        }
        if (readed_bytes < 0) {
            throw std::runtime_error(std::string(strerror(errno)));
        }
    } catch (std::runtime_error &ex) {
        _logger->error("Failed to process connection on descriptor {}: {}", _socket, ex.what());
    }
}

// See Connection.h
void Connection::DoWrite(std::shared_ptr<spdlog::logger> _logger) {
    _logger->debug("Writing");
    while (!_outgoing.empty()) {
        std::string &curr_msg = _outgoing.front();
        while(_head_offset < curr_msg.size()) {
            int sended_bytes = write(_socket, curr_msg.c_str() + _head_offset, curr_msg.size() - _head_offset);
            if (sended_bytes > 0 && sended_bytes != EWOULDBLOCK) {
                _head_offset += sended_bytes;
                continue;
            } else if (sended_bytes == EWOULDBLOCK) {
                return;
            } else {
                _logger->error("Failed to process connection on descriptor {}", _socket);
            }
        }

        _head_offset = 0;
        _outgoing.pop_front();
    }

    _event.events &= ~EPOLLOUT;
}

} // namespace STnonblock
} // namespace Network
} // namespace Afina
