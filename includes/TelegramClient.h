// TelegramClient.h
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Telegram Bot API client with retry logic

#ifndef TELEGRAM_CLIENT_H
#define TELEGRAM_CLIENT_H

#include <string>
#include <memory>

class Logger;

class TelegramClient {
public:
    TelegramClient(const std::string& api_key, const std::string& chat_id,
                   std::shared_ptr<Logger> logger);
    ~TelegramClient();

    // Send a message to Telegram (with retry logic)
    bool sendMessage(const std::string& message, int max_retries = 3);

    // Test if the bot configuration is valid
    bool testConnection();

private:
    std::string api_key_;
    std::string chat_id_;
    std::shared_ptr<Logger> logger_;

    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
    bool performRequest(const std::string& message, std::string& response);
    std::string escapeMessage(const std::string& message);
    void truncateIfNeeded(std::string& message);
};

#endif // TELEGRAM_CLIENT_H
