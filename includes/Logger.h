// Logger.h
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Thread-safe logging with rotation

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <mutex>
#include <fstream>

class Logger {
public:
    explicit Logger(const std::string& log_path, int keep_days = 3);
    ~Logger();

    // Log a message (thread-safe)
    void log(const std::string& message);

    // Log with different severity levels
    void info(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);

    // Rotate logs (remove old entries)
    void rotateLogs();

private:
    std::string log_path_;
    int keep_days_;
    std::mutex log_mutex_;

    std::string getTimestamp() const;
    void writeLog(const std::string& level, const std::string& message);
};

#endif // LOGGER_H
