// smtp2telegram.cpp (main)
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// This file is part of smtp-2-telegram.
// See LICENSE file in the project root for full license information.

#include "../includes/Config.h"
#include "../includes/Logger.h"
#include "../includes/TelegramClient.h"
#include "../includes/EmailParser.h"
#include "../includes/SMTPServer.h"
#include <iostream>
#include <memory>
#include <csignal>
#include <cstdlib>

// Global pointer for signal handler
std::shared_ptr<SMTPServer> g_server;
std::shared_ptr<Logger> g_logger;

void signalHandler(int signal) {
    if (g_logger) {
        g_logger->info("Received signal " + std::to_string(signal) + ", shutting down...");
    }
    if (g_server) {
        g_server->shutdown();
    }
}

int main() {
    try {
        // Load configuration
        Config config;
        config.load();

        // Create logger
        g_logger = std::make_shared<Logger>(config.getLogPath(), config.getLogKeepDays());
        g_logger->info("=== SMTP2Telegram Starting ===");
        g_logger->info("Configuration loaded successfully");

        // Rotate old logs
        g_logger->rotateLogs();

        // Create Telegram client
        auto telegram = std::make_shared<TelegramClient>(
            config.getApiKey(),
            config.getChatId(),
            g_logger
        );

        // Test Telegram connection
        g_logger->info("Testing Telegram connection...");
        if (!telegram->testConnection()) {
            g_logger->error("Failed to connect to Telegram. Please check your API_KEY and CHAT_ID.");
            return 1;
        }

        // Create email parser
        auto parser = std::make_shared<EmailParser>();

        // Create SMTP server
        g_server = std::make_shared<SMTPServer>(
            config.getSmtpHostname(),
            config.getSmtpPort(),
            telegram,
            g_logger,
            parser
        );

        // Set up signal handlers for graceful shutdown
        std::signal(SIGINT, signalHandler);
        std::signal(SIGTERM, signalHandler);

        g_logger->info("Signal handlers registered");

        // Run the server (blocking)
        g_server->run();

        g_logger->info("=== SMTP2Telegram Stopped ===");

    } catch (const ConfigException& e) {
        if (g_logger) {
            g_logger->error("Configuration error: " + std::string(e.what()));
        } else {
            std::cerr << "Configuration error: " << e.what() << std::endl;
        }
        return 1;
    } catch (const std::exception& e) {
        if (g_logger) {
            g_logger->error("Fatal error: " + std::string(e.what()));
        } else {
            std::cerr << "Fatal error: " << e.what() << std::endl;
        }
        return 1;
    }

    return 0;
}
