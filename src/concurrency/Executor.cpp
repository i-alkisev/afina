#include <afina/concurrency/Executor.h>

namespace Afina {
namespace Concurrency {

void Executor::Start() {
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
    
    state_ = State::kStopping;
    empty_condition_.notify_all(); 
    if (await) {
        while (working_threads_ > 0) {
            stopping_condition_.wait(lock);
        }
        state_ = State::kStopped;
    }
}


void perform(Executor *executor) {
    while (true) {
        std::function<void()> task;

        // Deciding what to do
        while (task == nullptr) {
            std::unique_lock<std::mutex> lock(executor->mutex_);
            if (!executor->tasks_.empty()){
                task = executor->tasks_.front();
                executor->tasks_.pop_front();
            }
            else if (executor->state_ == Executor::State::kStopping) {
                --executor->working_threads_;
                if (executor->working_threads_ == 0) {
                    executor->state_ = Executor::State::kStopped;
                    executor->stopping_condition_.notify_one();
                }
                return;
            }
            else {
                auto wait_status = executor->empty_condition_.wait_for(lock, executor->idle_time_);
                if (wait_status == std::cv_status::timeout && executor->working_threads_ > executor->low_watermark_) {
                    --executor->working_threads_;
                    return;
                }
            }
        }

        // Performing task
        try {
            task();
        }
        catch(...) {
            std::terminate();
        }
    }
}

} // namespace Concurrency
} // namespace Afina
