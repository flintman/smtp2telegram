// Queue.h
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Disk-backed message queue for smtp2telegram

#ifndef QUEUE_H
#define QUEUE_H

#include <string>
#include <vector>
#include <memory>

class Logger;

class Queue {
public:
    explicit Queue(const std::string& dir, std::shared_ptr<Logger> logger);
    bool enqueue(const std::string& message);
    std::vector<std::string> listFiles();
    std::string readFile(const std::string& filename);
    bool removeFile(const std::string& filename);
    std::string getDir() const { return dir_; }

private:
    std::string dir_;
    std::shared_ptr<Logger> logger_;
};

#endif // QUEUE_H
