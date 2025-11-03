// SMTPServer.cpp
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// SMTP server implementation

#include "../includes/SMTPServer.h"
#include "../includes/Logger.h"
#include "../includes/TelegramClient.h"
#include "../includes/EmailParser.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

using boost::asio::ip::tcp;

SMTPServer::SMTPServer(const std::string& hostname, int port,
                       std::shared_ptr<TelegramClient> telegram,
                       std::shared_ptr<Logger> logger,
                       std::shared_ptr<EmailParser> parser)
    : hostname_(hostname), port_(port), telegram_(telegram),
      logger_(logger), parser_(parser), shutdown_requested_(false) {
}

SMTPServer::~SMTPServer() {
}

void SMTPServer::shutdown() {
    shutdown_requested_ = true;
    logger_->info("Shutdown requested");
}

void SMTPServer::sendResponse(tcp::socket& socket, const std::string& response) {
    try {
        boost::asio::write(socket, boost::asio::buffer(response));
    } catch (const std::exception& e) {
        logger_->error("Failed to send response: " + std::string(e.what()));
    }
}

bool SMTPServer::readCommand(tcp::socket& socket, boost::asio::streambuf& buf, std::string& command) {
    try {
        boost::system::error_code ec;
        boost::asio::read_until(socket, buf, "\r\n", ec);

        if (ec) {
            if (ec != boost::asio::error::eof) {
                logger_->error("Error reading command: " + ec.message());
            }
            return false;
        }

        std::istream is(&buf);
        std::getline(is, command);
        if (!command.empty() && command.back() == '\r') {
            command.pop_back();
        }

        return true;
    } catch (const std::exception& e) {
        logger_->error("Exception reading command: " + std::string(e.what()));
        return false;
    }
}

bool SMTPServer::readData(tcp::socket& socket, boost::asio::streambuf& buf, std::string& data) {
    try {
        boost::system::error_code ec;
        boost::asio::read_until(socket, buf, "\r\n.\r\n", ec);

        if (ec) {
            logger_->error("Error reading DATA: " + ec.message());
            return false;
        }

        std::istream data_stream(&buf);
        std::ostringstream oss;
        oss << data_stream.rdbuf();
        data = oss.str();

        // Remove the terminating .\r\n
        if (data.length() >= 5) {
            data = data.substr(0, data.length() - 5);
        }

        return true;
    } catch (const std::exception& e) {
        logger_->error("Exception reading DATA: " + std::string(e.what()));
        return false;
    }
}

void SMTPServer::handleConnection(tcp::socket& socket) {
    try {
        tcp::endpoint remote_ep = socket.remote_endpoint();
        std::ostringstream conn_msg;
        conn_msg << "Connection from " << remote_ep.address().to_string()
                 << ":" << remote_ep.port();
        logger_->info(conn_msg.str());

        // Set socket timeout
        struct timeval tv;
        tv.tv_sec = 30;
        tv.tv_usec = 0;
        setsockopt(socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO,
                  (const char*)&tv, sizeof(tv));

        // Send greeting
        sendResponse(socket, "220 smtp2telegram ESMTP Service Ready\r\n");

        boost::asio::streambuf buf;
        std::string email_data;

        while (!shutdown_requested_) {
            buf.consume(buf.size());
            std::string cmd;

            if (!readCommand(socket, buf, cmd)) {
                break;
            }

            logger_->info("SMTP command: " + cmd);

            if (cmd.find("EHLO") == 0 || cmd.find("ehlo") == 0) {
                sendResponse(socket, "250-smtp2telegram greets you\r\n");
                sendResponse(socket, "250-PIPELINING\r\n");
                sendResponse(socket, "250-SIZE 35882577\r\n");
                sendResponse(socket, "250-8BITMIME\r\n");
                sendResponse(socket, "250-ENHANCEDSTATUSCODES\r\n");
                sendResponse(socket, "250-CHUNKING\r\n");
                sendResponse(socket, "250 HELP\r\n");
            } else if (cmd.find("HELO") == 0 || cmd.find("helo") == 0) {
                sendResponse(socket, "250 smtp2telegram greets you\r\n");
            } else if (cmd.find("MAIL FROM:") == 0 || cmd.find("mail from:") == 0) {
                sendResponse(socket, "250 OK\r\n");
            } else if (cmd.find("RCPT TO:") == 0 || cmd.find("rcpt to:") == 0) {
                sendResponse(socket, "250 OK\r\n");
            } else if (cmd == "DATA" || cmd == "data") {
                sendResponse(socket, "354 End data with <CR><LF>.<CR><LF>\r\n");

                if (readData(socket, buf, email_data)) {
                    // Parse and send email
                    ParsedEmail parsed = parser_->parse(email_data);
                    std::string telegram_msg = parser_->formatForTelegram(parsed);

                    if (!telegram_msg.empty()) {
                        if (telegram_->sendMessage(telegram_msg)) {
                            logger_->info("Email forwarded to Telegram");
                            sendResponse(socket, "250 OK: Message accepted\r\n");
                        } else {
                            logger_->error("Failed to forward email to Telegram");
                            sendResponse(socket, "451 Temporary failure\r\n");
                        }
                    } else {
                        logger_->warning("Empty email received");
                        sendResponse(socket, "250 OK: Empty message accepted\r\n");
                    }
                } else {
                    sendResponse(socket, "451 Requested action aborted: local error in processing\r\n");
                }
            } else if (cmd == "QUIT" || cmd == "quit") {
                sendResponse(socket, "221 Bye\r\n");
                break;
            } else if (cmd == "RSET" || cmd == "rset") {
                email_data.clear();
                sendResponse(socket, "250 OK\r\n");
            } else if (cmd == "NOOP" || cmd == "noop") {
                sendResponse(socket, "250 OK\r\n");
            } else if (cmd.empty()) {
                // Ignore empty commands
                continue;
            } else {
                // Unknown command, but be lenient
                logger_->warning("Unknown command: " + cmd);
                sendResponse(socket, "250 OK\r\n");
            }
        }

    } catch (const std::exception& e) {
        logger_->error("Connection error: " + std::string(e.what()));
    }
}

void SMTPServer::run() {
    try {
        boost::asio::io_context io_context;
        tcp::acceptor acceptor(io_context,
                               tcp::endpoint(boost::asio::ip::make_address(hostname_), port_));

        std::ostringstream listen_msg;
        listen_msg << "Starting SMTP server on " << hostname_ << ":" << port_;
        logger_->info(listen_msg.str());

        while (!shutdown_requested_) {
            tcp::socket socket(io_context);

            // Use async accept with timeout to allow checking shutdown flag
            boost::system::error_code ec;

            // Set acceptor to non-blocking mode
            acceptor.non_blocking(true);

            // Try to accept with polling
            while (!shutdown_requested_) {
                ec.clear();
                acceptor.accept(socket, ec);

                if (!ec) {
                    // Successfully accepted a connection
                    break;
                } else if (ec == boost::asio::error::would_block) {
                    // No connection yet, sleep briefly and check shutdown flag
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    continue;
                } else {
                    // Real error
                    logger_->error("Accept error: " + ec.message());
                    break;
                }
            }

            if (shutdown_requested_ || ec) {
                break;
            }

            handleConnection(socket);

            // Close socket
            try {
                socket.close();
            } catch (...) {
                // Ignore close errors
            }
        }

        logger_->info("SMTP server stopped");

    } catch (const std::exception& e) {
        logger_->error("Server error: " + std::string(e.what()));
        throw;
    }
}
