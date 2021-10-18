#include <afina/concurrency/Executor.h>

namespace Afina {
namespace Concurrency {

void Executor::Run() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (state_ == State::kRun) return;

    state_ = State::kRun;
    working_threads_ = low_watermark_;
    for (size_t i = 0; i < low_watermark_; ++i) {
        std::thread t(perform, this);
        t.detach();
    }
}


void Executor::Stop(bool await) {
    std::unique_lock<std::mutex> lock(mutex_);

    if (state_ == State::kStopped) return;
    
    if (await) {
        state_ = State::kStopped;
        empty_condition_.notify_all();
        return;
    }

    state_ = State::kStopping;
    empty_condition_.notify_all();
    while (working_threads_ > 0) {
        stopping_condition_.wait(lock);
    }
    state_ = State::kStopped;
}


void perform(Executor *executor) {
    while (true) {
        std::function<void()> task;

        // Deciding what to do
        bool was_busy = true;
        while (task == nullptr) {
            std::unique_lock<std::mutex> lock(executor->mutex_);
            if (!executor->tasks_.empty()){
                task = executor->tasks_.front();
                executor->tasks_.pop_front();
            }
            else {
                if (executor->state_ == Executor::State::kStopped) {
                    --executor->working_threads_;
                    return;
                }
                if (executor->state_ == Executor::State::kStopping) {
                    --executor->working_threads_;
                    if (executor->working_threads_ == 0) executor->stopping_condition_.notify_one();
                    return;
                }
                if (was_busy || executor->working_threads_ <= executor->low_watermark_) {
                    executor->empty_condition_.wait_for(lock, executor->idle_time_);
                    was_busy = false;
                }
                else {
                    --executor->working_threads_;
                    return;
                }
                
            }
        }

        // Performing task
        task();
    }
}

} // namespace Concurrency
} // namespace Afina
