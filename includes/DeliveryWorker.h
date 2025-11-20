// DeliveryWorker.h
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Background worker that flushes queued messages to Telegram

#ifndef DELIVERY_WORKER_H
#define DELIVERY_WORKER_H

#include <memory>
#include <atomic>
#include <thread>

class Queue;
class TelegramClient;
class Logger;

class DeliveryWorker {
public:
    DeliveryWorker(std::shared_ptr<Queue> queue,
                   std::shared_ptr<TelegramClient> telegram,
                   std::shared_ptr<Logger> logger,
                   int poll_seconds = 5);
    ~DeliveryWorker();

    void start();
    void stop();
    bool isRunning() const;

private:
    void runLoop();

    std::shared_ptr<Queue> queue_;
    std::shared_ptr<TelegramClient> telegram_;
    std::shared_ptr<Logger> logger_;
    int poll_seconds_;
    std::atomic<bool> running_;
    std::thread worker_thread_;
};

#endif // DELIVERY_WORKER_H