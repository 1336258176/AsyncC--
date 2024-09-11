# Mutex

## `shared_mutex`

- **共享锁**：允许多个线程同时持有锁，用于读操作，通过 `std::shared_lock<std::shared_mutex>` 实现；
- **互斥锁**：只允许一个线程持有锁，用于写操作，通过 `std::unique_lock<std::shared_mutex>` 实现；
- 当一个线程获得了互斥锁，就不再允许其他任何线程持有锁（包括共享锁），直至锁释放。
