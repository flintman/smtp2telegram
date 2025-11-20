// DeliveryWorker.cpp
// Copyright (c) 2024 William Bellavance Jr.
// SPDX-License-Identifier: MIT
//
// Background worker that flushes queued messages to Telegram

#include "../includes/DeliveryWorker.h"
#include "../includes/Queue.h"
#include "../includes/TelegramClient.h"
#include "../includes/Logger.h"
#include <chrono>
#include <thread>
#include <vector>

DeliveryWorker::DeliveryWorker(std::shared_ptr<Queue> queue,
                               std::shared_ptr<TelegramClient> telegram,
                               std::shared_ptr<Logger> logger,
                               int poll_seconds)
    : queue_(queue), telegram_(telegram), logger_(logger), poll_seconds_(poll_seconds), running_(false) {
}

DeliveryWorker::~DeliveryWorker() {
    stop();
}

void DeliveryWorker::start() {
    if (running_) return;
    running_ = true;
    worker_thread_ = std::thread(&DeliveryWorker::runLoop, this);
    if (logger_) logger_->info("Delivery worker started");
}

void DeliveryWorker::stop() {
    if (!running_) return;
    running_ = false;
    if (worker_thread_.joinable()) worker_thread_.join();
    if (logger_) logger_->info("Delivery worker stopped");
}

bool DeliveryWorker::isRunning() const {
    return running_;
}

void DeliveryWorker::runLoop() {
    while (running_) {
        try {
            if (!queue_) {
                std::this_thread::sleep_for(std::chrono::seconds(poll_seconds_));
                continue;
            }

            auto files = queue_->listFiles();

            if (files.empty()) {
                std::this_thread::sleep_for(std::chrono::seconds(poll_seconds_));
                continue;
            }

            for (const auto& f : files) {
                if (!running_) break;

                std::string msg = queue_->readFile(f);
                if (msg.empty()) {
                    // Corrupt or empty file: remove to avoid tight loops
                    queue_->removeFile(f);
                    continue;
                }

                if (!telegram_) {
                    if (logger_) logger_->error("No Telegram client available in DeliveryWorker");
                    break;
                }

                if (telegram_->sendMessage(msg)) {
                    if (logger_) logger_->info(std::string("Delivered queued message: ") + f);
                    queue_->removeFile(f);
                } else {
                    if (logger_) logger_->warning(std::string("Failed to deliver queued message: ") + f);
                    // On failure, don't remove; move to next file or sleep
                }

                // Small pause between messages to avoid tight looping
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

        } catch (const std::exception& e) {
            if (logger_) logger_->error(std::string("DeliveryWorker exception: ") + e.what());
        }

        // Poll interval
        std::this_thread::sleep_for(std::chrono::seconds(poll_seconds_));
    }
}
