#include <iostream>

#include "include/ThreadPool.hpp"

int add(int a, int b) { return a + b; }

class TestFunc {
 public:
  static double square(double x) { return x * x; }
};

struct Foo {
  template <typename T>
  T operator()(T a) {
    return a + 1;
  }
};

int main() {
  ThreadPool pool(4);
  auto res1 = pool.addTask(add, 1, 2);
  auto res2 = pool.addTask(TestFunc::square, 3.0);
  auto res3 = pool.addTask(Foo(), 4);
  // pool.addTask([]() { std::cout << "Hello, world!" << std::endl; });

  std::cout << "add(1, 2) = " << res1.get() << std::endl;
  std::cout << "square(3.0) = " << res2.get() << std::endl;
  std::cout << "Foo()(4) = " << res3.get() << std::endl;
  return 0;
}