// Logger.cpp
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Thread-safe logging implementation

#include "../includes/Logger.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <ctime>

Logger::Logger(const std::string& log_path, int keep_days)
    : log_path_(log_path), keep_days_(keep_days) {
}

Logger::~Logger() {
}

std::string Logger::getTimestamp() const {
    std::time_t now = std::time(nullptr);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

void Logger::log(const std::string& message) {
    writeLog("INFO", message);
}

void Logger::info(const std::string& message) {
    writeLog("INFO", message);
}

void Logger::warning(const std::string& message) {
    writeLog("WARN", message);
}

void Logger::error(const std::string& message) {
    writeLog("ERROR", message);
}

void Logger::writeLog(const std::string& level, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex_);

    std::string timestamp = getTimestamp();
    std::string log_entry = timestamp + " [" + level + "] - " + message;

    // Write to file
    std::ofstream log_file(log_path_, std::ios_base::app);
    if (log_file) {
        log_file << log_entry << std::endl;
        log_file.close();
    }

    // Also print to console
    std::cout << log_entry << std::endl;
}

void Logger::rotateLogs() {
    std::lock_guard<std::mutex> lock(log_mutex_);

    std::vector<std::string> lines;
    std::ifstream log_file_in(log_path_);

    if (!log_file_in) return; // No log file yet

    std::time_t now = std::time(nullptr);
    const std::time_t cutoff = now - keep_days_ * 24 * 60 * 60;
    std::string line;

    // Read and filter old log lines
    while (std::getline(log_file_in, line)) {
        if (line.size() < 19) {
            lines.push_back(line); // Keep malformed lines
            continue;
        }

        std::tm tm = {};
        std::istringstream iss(line.substr(0, 19));
        iss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

        if (iss.fail()) {
            lines.push_back(line); // Keep lines that can't be parsed
            continue;
        }

        std::time_t log_time = std::mktime(&tm);
        if (log_time >= cutoff) {
            lines.push_back(line);
        }
    }
    log_file_in.close();

    // Write filtered logs back
    std::ofstream log_file_out(log_path_, std::ios_base::trunc);
    if (log_file_out) {
        for (const auto& l : lines) {
            log_file_out << l << std::endl;
        }
        log_file_out.close();
    }
}
