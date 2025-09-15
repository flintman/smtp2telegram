// smtp2telegram.h
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Function prototypes and documentation for smtp2telegram

#ifndef SMTP2TELEGRAM_H
#define SMTP2TELEGRAM_H

#include <string>

// Log a message to file and stdout
void log(const std::string& msg);

// Send a message to Telegram using the Bot API
bool send_telegram_message(const std::string& api_key, const std::string& chat_id, const std::string& message);

// Run the SMTP server loop
void smtp_server(const std::string& hostname, int port, const std::string& api_key, const std::string& chat_id);

#endif // SMTP2TELEGRAM_H