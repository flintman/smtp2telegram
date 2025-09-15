// smtp2telegram.cpp
// Copyright (c) 2024 William Bellavance Jr. 
// SPDX-License-Identifier: MIT
//
// This file is part of smtp-2-telegram.
// See LICENSE file in the project root for full license information.

#include <iostream>
#include <fstream>
#include <cstdlib>
#include <string>
#include <ctime>
#include <boost/asio.hpp>
#include <curl/curl.h>
#include <iomanip>

// Logging utility
void log(const std::string& msg) {
    std::ofstream log_file("smtp_server.log", std::ios_base::app);
    std::time_t now = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    std::string timestamp = oss.str();
    log_file << timestamp << " - " << msg << std::endl;
    std::cout << timestamp << " - " << msg << std::endl;
}

// Send message to Telegram
bool send_telegram_message(const std::string& api_key, const std::string& chat_id, const std::string& message) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    std::string url = "https://api.telegram.org/bot" + api_key +
                      "/sendMessage?chat_id=" + chat_id +
                      "&text=" + curl_easy_escape(curl, message.c_str(), message.length());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); // Disable SSL verification (not recommended for production)
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    log("Telegram message sent: " + message);
    return (res == CURLE_OK);
}

// Minimal SMTP handler (stub, not RFC-compliant)
void smtp_server(const std::string& hostname, int port, const std::string& api_key, const std::string& chat_id) {
    using boost::asio::ip::tcp;
    boost::asio::io_context io_context;
    tcp::acceptor acceptor(io_context, tcp::endpoint(boost::asio::ip::make_address(hostname), port));

    std::ostringstream listen_msg;
    listen_msg << "Starting SMTP server on " << hostname << ":" << port;
    log(listen_msg.str());
    while (true) {
        tcp::socket socket(io_context);
        acceptor.accept(socket);

        // Log incoming connection details
        boost::asio::ip::tcp::endpoint remote_ep = socket.remote_endpoint();
        std::ostringstream conn_msg;
        conn_msg << "Connection from " << remote_ep.address().to_string() << ":" << remote_ep.port();
        log(conn_msg.str());

        // Send SMTP greeting to client
        std::string greeting = "220 smtp2telegram ESMTP Service Ready\r\n";
        boost::asio::write(socket, boost::asio::buffer(greeting));

        boost::asio::streambuf buf;
        boost::system::error_code ec;
        struct timeval tv;
        tv.tv_sec = 30;  // 30 seconds
        tv.tv_usec = 0;
        setsockopt(socket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);

        // Minimal SMTP state machine
        auto send_response = [&](const std::string& resp) {
            boost::asio::write(socket, boost::asio::buffer(resp));
        };

        std::string subject, content;
        bool got_data = false;
        while (!got_data) {
            buf.consume(buf.size()); // Clear buffer
            boost::asio::read_until(socket, buf, "\r\n", ec);
            if (ec) break;
            std::istream is(&buf);
            std::string cmd;
            std::getline(is, cmd);
            if (!cmd.empty() && cmd.back() == '\r') cmd.pop_back();
            log("SMTP command: " + cmd);

            // Log all received SMTP commands
            log("Received SMTP command: " + cmd);

            if (cmd.find("EHLO") == 0) {
                // Respond with standard SMTP extensions
                send_response("250-smtp2telegram greets you\r\n");
                send_response("250-PIPELINING\r\n");
                send_response("250-SIZE 35882577\r\n");
                send_response("250-8BITMIME\r\n");
                send_response("250-ENHANCEDSTATUSCODES\r\n");
                send_response("250-CHUNKING\r\n");
                send_response("250 HELP\r\n");
            } else if (cmd.find("HELO") == 0) {
                send_response("250 smtp2telegram greets you\r\n");
            } else if (cmd.find("MAIL FROM:") == 0) {
                send_response("250 OK\r\n");
            } else if (cmd.find("RCPT TO:") == 0) {
                send_response("250 OK\r\n");
            } else if (cmd == "DATA") {
                send_response("354 End data with <CR><LF>.<CR><LF>\r\n");
                // Read message data until \r\n.\r\n
                boost::asio::streambuf data_buf;
                boost::asio::read_until(socket, data_buf, "\r\n.\r\n", ec);
                if (ec) {
                    log(std::string("Error reading SMTP DATA: ") + ec.message());
                    send_response("451 Requested action aborted: local error in processing\r\n");
                    break;
                }
                std::istream data_stream(&data_buf);
                std::string line;
                bool in_headers = true;
                while (std::getline(data_stream, line)) {
                    if (!line.empty() && line.back() == '\r') line.pop_back();
                    if (in_headers && line.find("Subject:") == 0) subject = line.substr(8);
                    if (line.empty()) in_headers = false;
                    if (!in_headers) content += line + "\n";
                }
                got_data = true;
            } else if (cmd == "QUIT") {
                send_response("221 Bye\r\n");
                break;
            } else {
                send_response("250 OK\r\n");
            }
        }

        // Clean up content
        size_t pos;
        while ((pos = content.find("=0A")) != std::string::npos)
            content.replace(pos, 3, "\n");

        if (!subject.empty() || !content.empty()) {
            std::string msg = subject + "\n" + content;
            send_telegram_message(api_key, chat_id, msg);
            log("Email processed: Subject: " + subject + ", Content: " + content);
            send_response("250 OK\r\n");
        }
    }
}

int main() {
    // Read config from environment
    // Load environment variables from .env file if present
    std::ifstream env_file(".env");
    if (env_file) {
        std::string line;
        while (std::getline(env_file, line)) {
            size_t eq = line.find('=');
            if (eq != std::string::npos) {
                std::string key = line.substr(0, eq);
                std::string value = line.substr(eq + 1);
                setenv(key.c_str(), value.c_str(), 1);
            }
        }
    }

    const char* chat_id = std::getenv("CHAT_ID");
    const char* api_key = std::getenv("API_KEY");
    const char* hostname = std::getenv("SMTP_HOSTNAME");
    const char* port_str = std::getenv("SMTP_PORT");

    if (!chat_id || !api_key || !hostname || !port_str) {
        std::cerr << "Missing environment variables. Set CHAT_ID, API_KEY, SMTP_HOSTNAME, SMTP_PORT." << std::endl;
        return 1;
    }

    int port = std::stoi(port_str);

    try {
        smtp_server(hostname, port, api_key, chat_id);
    } catch (const std::exception& e) {
        log(std::string("Error: ") + e.what());
        std::cerr << "Server error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}