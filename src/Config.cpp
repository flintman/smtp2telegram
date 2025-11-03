// Config.cpp
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Configuration management implementation

#include "../includes/Config.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <sys/stat.h>
#include <cerrno>
#include <cstring>

Config::Config()
    : smtp_port_(2525), log_keep_days_(3) {
    const char* home = std::getenv("HOME");
    if (!home) {
        throw ConfigException("HOME environment variable not set");
    }

    config_dir_ = std::string(home) + "/smtp2telegram";
    env_path_ = config_dir_ + "/.env";
    log_path_ = config_dir_ + "/smtp_server.log";
}

void Config::load() {
    createConfigDirectory();

    std::ifstream env_file(env_path_);
    if (!env_file) {
        createEnvFile();
        env_file.open(env_path_);
    }

    if (env_file) {
        loadEnvFile();
    }

    // Load from environment variables
    const char* chat_id = std::getenv("CHAT_ID");
    const char* api_key = std::getenv("API_KEY");
    const char* hostname = std::getenv("SMTP_HOSTNAME");
    const char* port_str = std::getenv("SMTP_PORT");
    const char* log_keep_days_str = std::getenv("LOG_KEEP_DAYS");

    if (!chat_id || !api_key || !hostname || !port_str || !log_keep_days_str) {
        throw ConfigException("Missing required environment variables. Set CHAT_ID, API_KEY, SMTP_HOSTNAME, SMTP_PORT, LOG_KEEP_DAYS.");
    }

    chat_id_ = chat_id;
    api_key_ = api_key;
    smtp_hostname_ = hostname;

    try {
        smtp_port_ = std::stoi(port_str);
        log_keep_days_ = std::stoi(log_keep_days_str);
    } catch (const std::exception& e) {
        throw ConfigException("Invalid numeric configuration value: " + std::string(e.what()));
    }

    if (!validate()) {
        throw ConfigException("Configuration validation failed");
    }

    setSecurePermissions();
}

void Config::createConfigDirectory() {
    struct stat st = {0};
    if (stat(config_dir_.c_str(), &st) == -1) {
        if (mkdir(config_dir_.c_str(), 0700) == -1) {
            throw ConfigException("Failed to create directory " + config_dir_ + ": " + strerror(errno));
        }
    }
}

void Config::createEnvFile() {
    std::cout << ".env file not found. Please provide the following information:\n";
    std::string chat_id, api_key, smtp_hostname, smtp_port, log_keep_days;

    std::cout << "CHAT_ID (Telegram chat ID): ";
    std::getline(std::cin, chat_id);

    std::cout << "API_KEY (Telegram bot API key): ";
    std::getline(std::cin, api_key);

    std::cout << "SMTP_HOSTNAME (SMTP server hostname, e.g., 0.0.0.0): ";
    std::getline(std::cin, smtp_hostname);

    std::cout << "SMTP_PORT (SMTP server port, e.g., 2525): ";
    std::getline(std::cin, smtp_port);

    std::cout << "LOG_KEEP_DAYS (Days to keep logs, default 3): ";
    std::getline(std::cin, log_keep_days);

    // Provide defaults
    if (smtp_hostname.empty()) smtp_hostname = "0.0.0.0";
    if (smtp_port.empty()) smtp_port = "2525";
    if (log_keep_days.empty()) log_keep_days = "3";

    std::ofstream new_env(env_path_);
    if (new_env) {
        new_env << "CHAT_ID=" << chat_id << "\n";
        new_env << "API_KEY=" << api_key << "\n";
        new_env << "SMTP_HOSTNAME=" << smtp_hostname << "\n";
        new_env << "SMTP_PORT=" << smtp_port << "\n";
        new_env << "LOG_KEEP_DAYS=" << log_keep_days << "\n";
        new_env.close();
        std::cout << ".env file created at " << env_path_ << "\n";

        // Set secure permissions immediately
        chmod(env_path_.c_str(), 0600);
    } else {
        throw ConfigException("Failed to create .env file at " + env_path_);
    }
}

void Config::loadEnvFile() {
    std::ifstream env_file(env_path_);
    std::string line;
    while (std::getline(env_file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        size_t eq = line.find('=');
        if (eq != std::string::npos) {
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);

            // Trim whitespace
            key.erase(0, key.find_first_not_of(" \t\r\n"));
            key.erase(key.find_last_not_of(" \t\r\n") + 1);
            value.erase(0, value.find_first_not_of(" \t\r\n"));
            value.erase(value.find_last_not_of(" \t\r\n") + 1);

            setenv(key.c_str(), value.c_str(), 1);
        }
    }
}

void Config::setSecurePermissions() {
    // Ensure .env file has restricted permissions
    chmod(env_path_.c_str(), 0600);
}

bool Config::validate() const {
    if (chat_id_.empty()) {
        std::cerr << "Error: CHAT_ID cannot be empty\n";
        return false;
    }

    if (api_key_.empty()) {
        std::cerr << "Error: API_KEY cannot be empty\n";
        return false;
    }

    if (smtp_hostname_.empty()) {
        std::cerr << "Error: SMTP_HOSTNAME cannot be empty\n";
        return false;
    }

    if (!validatePort(smtp_port_)) {
        std::cerr << "Error: SMTP_PORT must be between 1 and 65535\n";
        return false;
    }

    if (log_keep_days_ < 1) {
        std::cerr << "Error: LOG_KEEP_DAYS must be at least 1\n";
        return false;
    }

    if (!validateChatId(chat_id_)) {
        std::cerr << "Warning: CHAT_ID format may be invalid\n";
    }

    return true;
}

bool Config::validatePort(int port) const {
    return port > 0 && port <= 65535;
}

bool Config::validateChatId(const std::string& chat_id) const {
    // Telegram chat IDs are typically numeric (can be negative for groups)
    if (chat_id.empty()) return false;

    size_t start = 0;
    if (chat_id[0] == '-') start = 1;

    for (size_t i = start; i < chat_id.length(); ++i) {
        if (!isdigit(chat_id[i])) return false;
    }

    return true;
}
