// Config.h
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Configuration management for smtp2telegram

#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <stdexcept>

class ConfigException : public std::runtime_error {
public:
    explicit ConfigException(const std::string& message) : std::runtime_error(message) {}
};

class Config {
public:
    Config();

    // Load configuration from environment or create .env file
    void load();

    // Validate configuration values
    bool validate() const;

    // Getters
    std::string getChatId() const { return chat_id_; }
    std::string getApiKey() const { return api_key_; }
    std::string getSmtpHostname() const { return smtp_hostname_; }
    int getSmtpPort() const { return smtp_port_; }
    int getLogKeepDays() const { return log_keep_days_; }
    std::string getConfigDir() const { return config_dir_; }
    std::string getLogPath() const { return log_path_; }

private:
    std::string config_dir_;
    std::string env_path_;
    std::string log_path_;

    std::string chat_id_;
    std::string api_key_;
    std::string smtp_hostname_;
    int smtp_port_;
    int log_keep_days_;

    void createConfigDirectory();
    void createEnvFile();
    void loadEnvFile();
    void setSecurePermissions();
    bool validatePort(int port) const;
    bool validateChatId(const std::string& chat_id) const;
};

#endif // CONFIG_H
