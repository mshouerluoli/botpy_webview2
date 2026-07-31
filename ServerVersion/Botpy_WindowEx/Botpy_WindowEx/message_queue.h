#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>

struct MessageTask {
    std::string id;
    std::string content;
    std::string sender_id;
    std::string channel_id;
    bool is_group;
    bool is_groupat;
    std::string openid;
    std::string group_openid;
};

class MessageQueue {
public:
    MessageQueue() : m_running(true) {}

    void push(const MessageTask& task) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (!m_running) return;
            m_queue.push(task);
        }
        m_cv.notify_one();
    }

    bool pop(MessageTask& task) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this]() {
            return !m_queue.empty() || !m_running;
        });
        if (!m_running && m_queue.empty()) {
            return false;
        }
        task = m_queue.front();
        m_queue.pop();
        return true;
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_running = false;
        }
        m_cv.notify_all();
    }

    void reset() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = true;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }

private:
    std::queue<MessageTask> m_queue;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    bool m_running;
};
