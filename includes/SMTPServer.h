// SMTPServer.h
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// SMTP server with signal handling

#ifndef SMTP_SERVER_H
#define SMTP_SERVER_H

#include <string>
#include <memory>
#include <atomic>
#include <boost/asio.hpp>

class Logger;
class TelegramClient;
class EmailParser;

class SMTPServer {
public:
    SMTPServer(const std::string& hostname, int port,
               std::shared_ptr<TelegramClient> telegram,
               std::shared_ptr<Logger> logger,
               std::shared_ptr<EmailParser> parser);
    ~SMTPServer();

    // Start the server (blocking)
    void run();

    // Request graceful shutdown
    void shutdown();

    // Check if shutdown was requested
    bool isShutdownRequested() const { return shutdown_requested_; }

private:
    std::string hostname_;
    int port_;
    std::shared_ptr<TelegramClient> telegram_;
    std::shared_ptr<Logger> logger_;
    std::shared_ptr<EmailParser> parser_;
    std::atomic<bool> shutdown_requested_;

    void handleConnection(boost::asio::ip::tcp::socket& socket);
    void sendResponse(boost::asio::ip::tcp::socket& socket, const std::string& response);
    bool readCommand(boost::asio::ip::tcp::socket& socket,
                     boost::asio::streambuf& buf,
                     std::string& command);
    bool readData(boost::asio::ip::tcp::socket& socket,
                  boost::asio::streambuf& buf,
                  std::string& data);
};

#endif // SMTP_SERVER_H
