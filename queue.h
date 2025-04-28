#ifndef QUEUE_H
#define QUEUE_H

#include <string>
#include <queue>
#include <omp.h>

class SharedQueue {
private:
    std::queue<std::string> queue;
    omp_lock_t lock;
    bool done;

public:
    SharedQueue() {
        omp_init_lock(&lock);
        done = false;
    }

    ~SharedQueue() {
        omp_destroy_lock(&lock);
    }

    void push(const std::string& line) {
        omp_set_lock(&lock);
        queue.push(line);
        omp_unset_lock(&lock);
    }

    bool pop(std::string& line) {
        bool success = false;
        omp_set_lock(&lock);
        if (!queue.empty()) {
            line = queue.front();
            queue.pop();
            success = true;
        }
        omp_unset_lock(&lock);
        return success;
    }

    void setDone() {
        omp_set_lock(&lock);
        done = true;
        omp_unset_lock(&lock);
    }

    bool isDone() {
        bool status;
        omp_set_lock(&lock);
        status = done && queue.empty();
        omp_unset_lock(&lock);
        return status;
    }
};

#endif