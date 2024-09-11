#ifndef THREADPOOL_HPP__
#define THREADPOOL_HPP__

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

using Task = std::function<void()>;

class ThreadPool {
 public:
  unsigned int MaxThreadNum = std::thread::hardware_concurrency();

  ThreadPool(unsigned int thread_num = 4) : thread_num_(thread_num) {
    thread_num_ = thread_num_ > MaxThreadNum ? MaxThreadNum : thread_num_;
    idle_thread_num_ = thread_num_;
    for (unsigned int i = 0; i < thread_num_; i++) {
      threads_.emplace_back([this]() {
        while (true) {
          Task task;
          {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this] {
              return (!tasks_.empty() || !is_running) && (idle_thread_num_ != 0);
            });
            if (!is_running && tasks_.empty()) {
              return;
            }
            task = std::move(tasks_.front());
            tasks_.pop();
          }
          idle_thread_num_--;
          task();
          idle_thread_num_++;
        }
      });
    }
  }

  ~ThreadPool() {
    is_running = false;
    cv_.notify_all();
    for (auto& thread : threads_) {
      if (thread.joinable()) {
        thread.join();
      }
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  template <typename Func, typename... Args>
  auto addTask(Func&& func, Args&&... args) -> std::future<decltype(func(args...))> {
    if (!is_running) {
      throw std::runtime_error("ThreadPool is stopped.");
    }

    using ReturnType = decltype(func(args...));
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
    auto future = task->get_future();
    {
      std::unique_lock<std::mutex> lock(mutex_);
      tasks_.emplace([task]() { (*task)(); });
    }
    cv_.notify_one();
    return future;
  }

  unsigned int getThreadNum() const { return thread_num_; }

  unsigned int getIdleThreadNum() const { return idle_thread_num_; }

 private:
  std::queue<Task> tasks_;
  std::vector<std::thread> threads_;

  std::mutex mutex_{};
  std::condition_variable cv_{};

  std::atomic<bool> is_running{true};
  unsigned int thread_num_{0};
  std::atomic<unsigned int> idle_thread_num_{0};
};

#endif  // THREADPOOL_HPP__
