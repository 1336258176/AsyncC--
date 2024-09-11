# ThreadPool

## 任务队列的设计

任务队列的成员最好设计成`std::function<void()>`的形式，这种形式的好处在于可以以一个无返回值且无参的匿名函数作为其成员，而该匿名函数的函数体就是执行传递的任务。还有一个问题就是STL标准库中的`std::queue`不是线程安全的，因此对于队列的读写操作需要加锁。

## 可调用类型的传递

1. 右值引用的可变参数模板：方便接收临时对象和可调用对象以实现完美转发；
2. 可调用类型的包装：将函数与参数绑定，打包成`std::packaged_task`，因为此时提交的任务不能立刻返回结果，需要获取`std::future`；
3. 以`std::packaged_task`获取的`std::future`作为函数返回值；
4. 将一个执行可调用对象参数的匿名函数添加到任务队列中。

```cpp
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
```

## 线程池的运转

1. 由于不清楚任务加载的时间，所以要使线程一直运行下去，即`while(true)`；

```cpp
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
```
