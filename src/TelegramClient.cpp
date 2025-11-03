// TelegramClient.cpp
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Telegram Bot API client implementation

#include "../includes/TelegramClient.h"
#include "../includes/Logger.h"
#include <curl/curl.h>
#include <thread>
#include <chrono>

// Telegram message limit is 4096 characters
const size_t TELEGRAM_MESSAGE_LIMIT = 4096;

TelegramClient::TelegramClient(const std::string& api_key, const std::string& chat_id,
                               std::shared_ptr<Logger> logger)
    : api_key_(api_key), chat_id_(chat_id), logger_(logger) {
}

TelegramClient::~TelegramClient() {
}

size_t TelegramClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string TelegramClient::escapeMessage(const std::string& message) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    char* escaped = curl_easy_escape(curl, message.c_str(), message.length());
    std::string result = escaped ? escaped : "";

    if (escaped) curl_free(escaped);
    curl_easy_cleanup(curl);

    return result;
}

void TelegramClient::truncateIfNeeded(std::string& message) {
    if (message.length() > TELEGRAM_MESSAGE_LIMIT) {
        message = message.substr(0, TELEGRAM_MESSAGE_LIMIT - 50);
        message += "\n\n... (message truncated)";
    }
}

bool TelegramClient::performRequest(const std::string& message, std::string& response) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        logger_->error("Failed to initialize CURL");
        return false;
    }

    std::string truncated_msg = message;
    truncateIfNeeded(truncated_msg);

    std::string escaped_message = escapeMessage(truncated_msg);
    if (escaped_message.empty()) {
        curl_easy_cleanup(curl);
        logger_->error("Failed to escape message");
        return false;
    }

    std::string url = "https://api.telegram.org/bot" + api_key_ +
                      "/sendMessage?chat_id=" + chat_id_ +
                      "&text=" + escaped_message;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        logger_->error("Telegram API request failed: " + std::string(curl_easy_strerror(res)));
        return false;
    }

    if (response_code != 200) {
        logger_->error("Telegram API returned HTTP " + std::to_string(response_code) + ": " + response);
        return false;
    }

    return true;
}

bool TelegramClient::sendMessage(const std::string& message, int max_retries) {
    for (int attempt = 1; attempt <= max_retries; ++attempt) {
        std::string response;

        if (performRequest(message, response)) {
            logger_->info("Telegram message sent successfully");
            return true;
        }

        if (attempt < max_retries) {
            int wait_seconds = attempt * 2; // Exponential backoff
            logger_->warning("Retry " + std::to_string(attempt) + "/" +
                           std::to_string(max_retries) + " in " +
                           std::to_string(wait_seconds) + " seconds...");
            std::this_thread::sleep_for(std::chrono::seconds(wait_seconds));
        }
    }

    logger_->error("Failed to send Telegram message after " + std::to_string(max_retries) + " attempts");
    return false;
}

bool TelegramClient::testConnection() {
    logger_->info("Testing Telegram bot connection...");
    return sendMessage("smtp2telegram: Connection test successful", 1);
}
