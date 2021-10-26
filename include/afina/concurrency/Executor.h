#ifndef AFINA_CONCURRENCY_EXECUTOR_H
#define AFINA_CONCURRENCY_EXECUTOR_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <chrono>

namespace Afina {
namespace Concurrency {

class Executor;

void perform(Executor *executor);
/**
 * # Thread pool
 */
class Executor {
    enum class State {
        // Threadpool is fully operational, tasks could be added and get executed
        kRun,

        // Threadpool is on the way to be shutdown, no ned task could be added, but existing will be
        // completed as requested
        kStopping,

        // Threadppol is stopped
        kStopped
    };

public:
    Executor(size_t low_watermark, size_t hight_watermark, size_t max_queue_size, std::chrono::milliseconds idle_time):
    working_threads_(0),
    state_(State::kStopped),
    low_watermark_(low_watermark),
    hight_watermark_(hight_watermark),
    max_queue_size_(max_queue_size),
    idle_time_(idle_time) {}

    ~Executor() { Stop(true); }

    void Start();

    /**
     * Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
     * free. All enqueued jobs will be complete.
     *
     * In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
     */
    void Stop(bool await = false);

    /**
     * Add function to be executed on the threadpool. Method returns true in case if task has been placed
     * onto execution queue, i.e scheduled for execution and false otherwise.
     *
     * That function doesn't wait for function result. Function could always be written in a way to notify caller about
     * execution finished by itself
     */
    template <typename F, typename... Types> bool Execute(F &&func, Types... args) {
        // Prepare "task"
        auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);

        std::unique_lock<std::mutex> lock(this->mutex_);
        if (state_ != State::kRun || tasks_.size() == max_queue_size_) {
            return false;
        }

        // Enqueue new task
        tasks_.push_back(exec);

        // Create new thread if queue wasn't empty and hight_watermark_ isn't reached
        if (tasks_.size() > working_threads_ && working_threads_ < hight_watermark_) {
            ++working_threads_;
            std::thread t(perform, this);
            t.detach();
        }
        else {
            empty_condition_.notify_one();
        }
        return true;
    }

private:
    // No copy/move/assign allowed
    Executor(const Executor &);            // = delete;
    Executor(Executor &&);                 // = delete;
    Executor &operator=(const Executor &); // = delete;
    Executor &operator=(Executor &&);      // = delete;

    /**
     * Main function that all pool threads are running. It polls internal task queue and execute tasks
     */
    friend void perform(Executor *executor);

    /**
     * Mutex to protect state below from concurrent modification
     */
    std::mutex mutex_;

    /**
     * Conditional variable to await new data in case of empty queue
     */
    std::condition_variable empty_condition_;

    /**
     * Count working threads
     */
    size_t working_threads_;

    /**
     * Task queue
     */
    std::deque<std::function<void()>> tasks_;

    /**
     * Flag to stop bg threads
     */
    State state_;

    size_t low_watermark_, hight_watermark_, max_queue_size_;
    std::chrono::milliseconds idle_time_;
    std::condition_variable stopping_condition_;
};

} // namespace Concurrency
} // namespace Afina
#endif // AFINA_CONCURRENCY_EXECUTOR_H
