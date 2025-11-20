// Queue.cpp
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Simple disk-backed queue implementation

#include "../includes/Queue.h"
#include "../includes/Logger.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <random>
#include <thread>
#include <algorithm>

namespace fs = std::filesystem;

Queue::Queue(const std::string& dir, std::shared_ptr<Logger> logger)
    : dir_(dir), logger_(logger) {
    try {
        fs::create_directories(dir_);
    } catch (...) {
        if (logger_) logger_->error("Failed to create queue directory: " + dir_);
    }
}

bool Queue::enqueue(const std::string& message) {
    try {
        // filename: epoch_pid_rand.msg
        auto now = std::chrono::system_clock::now();
        auto epoch = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::ostringstream oss;
        oss << epoch << "_" << std::this_thread::get_id();
        // add random suffix
        std::random_device rd;
        oss << "_" << rd();
        std::string tmp = dir_ + "/." + oss.str() + ".tmp";
        std::string finalp = dir_ + "/" + oss.str() + ".msg";

        std::ofstream ofs(tmp, std::ios::binary);
        if (!ofs) {
            if (logger_) logger_->error("Failed to open temp queue file: " + tmp);
            return false;
        }
        ofs << message;
        ofs.close();
        fs::rename(tmp, finalp);
        if (logger_) logger_->info("Enqueued message to " + finalp);
        return true;
    } catch (const std::exception& e) {
        if (logger_) logger_->error(std::string("Enqueue failed: ") + e.what());
        return false;
    }
}

std::vector<std::string> Queue::listFiles() {
    std::vector<std::string> files;
    try {
        for (auto& p : fs::directory_iterator(dir_)) {
            if (!p.is_regular_file()) continue;
            std::string name = p.path().filename().string();
            if (name.size() > 4 && name.substr(name.size()-4) == ".msg") {
                files.push_back(name);
            }
        }
        // optional: sort by name -> timestamp order
        std::sort(files.begin(), files.end());
    } catch (const std::exception& e) {
        if (logger_) logger_->error(std::string("Queue listFiles error: ") + e.what());
    }
    return files;
}

std::string Queue::readFile(const std::string& filename) {
    std::string path = dir_ + "/" + filename;
    try {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs) return std::string();
        std::ostringstream oss;
        oss << ifs.rdbuf();
        return oss.str();
    } catch (...) {
        return std::string();
    }
}

bool Queue::removeFile(const std::string& filename) {
    std::string path = dir_ + "/" + filename;
    try {
        return fs::remove(path);
    } catch (const std::exception& e) {
        if (logger_) logger_->error(std::string("Failed to remove queue file: ") + e.what());
        return false;
    }
}
