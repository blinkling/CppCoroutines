//
// Created by benny on 2022/3/17.
//
#include "Executor.h"
#include "Task.h"
#include "io_utils.h"
#include "Scheduler.h"

void test_scheduler() {
  auto scheduler = Scheduler();

  debug("start")
  scheduler.execute([]() { debug("1"); }, 50);
  scheduler.execute([]() { debug("2"); }, 100);
  scheduler.execute([]() { debug("3"); }, 200);
  scheduler.execute([]() { debug("4"); }, 300);
  scheduler.execute([]() { debug("5"); }, 500);
  scheduler.execute([]() { debug("6"); }, 1000);

  scheduler.shutdown();
}

/*
Task模板类的第二个参数, 提供了执行任务的机制(Executor)
它以组合的方式, 被Task内部的promise_type持有.

它具体在哪儿执行呢:

观察promise_type的函数:
template<typename _ResultType, typename _Executor>
TaskAwaiter<_ResultType, _Executor> await_transform(Task<_ResultType, _Executor> &&task) {
    return TaskAwaiter<_ResultType, _Executor>(&executor, std::move(task));
}

executor会被传递给一个等待体(DispatchAwaiter).
等待体的:
  void await_suspend(std::coroutine_handle<> handle) const {
    _executor->execute([handle]() {
      handle.resume();
    });
  }
会用_executor调用execute执行真正需要执行的任务.

所以这里的逻辑:
每次 co_await  一个Task的时候, 这个Task就会去执行.


细致的捋一下:
1. 
auto simpleTask = simple_task(); //①
运行的时候, 之前有学到过, 一个携程体被执行的时候, 第一步就是创建携程体, 也就是这里的 Task<int, LooperExecutor>
    创建的时候会去调用它的 get_return_object函数
    - get_return_object():
    返回与此协程关联的外部对象（通常是由用户直接使用的对象，例如 task 或 generator）。它是协程开始执行后第一个被调用的成员函数。
    完成后开始调用 initial_suspend , 观察这里的initial_suspend
    DispatchAwaiter initial_suspend() { return DispatchAwaiter{&executor}; }
    initial_suspend返回DispatchAwaiter的时候
    DispatchAwaiter开始了它的逻辑:
    如果await_ready返回的是false, 那就挂起去执行await_suspend.
        void await_suspend(std::coroutine_handle<> handle) const {
        _executor->execute([handle]() {
        handle.resume();
          });
        }
      这里挂起去执行_executor->execute的时候, execute会按照它的逻辑,执行传入的lambda
      如果excute是异步的, 那么因为内部调用了handle.resume(),handle.resume()会主动调用await_resume()这里有需要还可以在其中添加自己的逻辑, 然后携程体恢复到了挂起点,此时已经切换了
      线程来执行了. 因为是在这个新的线程里resume的(executor的execute决定的).
        这个时候的挂起点就是simple_task()这个携程体最开始的位置,(我都理解它是隐式的,因为不是通过co_await)而是通过携程体创建时候的机制(上面讨论的initial_suspend)
2. 接着前面提到的resume的线程中, 往下执行到了②:
   auto result2 = co_await simple_task2();//②
   这就是在执行要给新的携程体了, 又重复了之前的操作:
   get_return_object创建 Task<int, AsyncExecutor>
   调用它的initial_suspend,
   判断DispatchAwaiter的await_ready , ok 还是false 挂起:
   执行await_suspend, 注意这里的这次调用的是AsyncExecutor了, 多态没啥好说的了, 执行它的execute逻辑
   
   剩下的没什么说的了,都是这个逻辑.


*/

Task<int, AsyncExecutor> simple_task2() {
  debug("task 2 start ...");
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1s);
  debug("task 2 returns after 1s.");
  co_return 2;
}

Task<int, NewThreadExecutor> simple_task3() {
  debug("in task 3 start ...");
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);
  debug("task 3 returns after 2s.");
  co_return 3;
}

Task<int, LooperExecutor> simple_task() {
  debug("task start ...");
  auto result2 = co_await simple_task2();//②
  debug("returns from task2: ", result2);
  auto result3 = co_await simple_task3();
  debug("returns from task3: ", result3);
  co_return 1 + result2 + result3;
}

void test_tasks() {
  auto simpleTask = simple_task();//①
  simpleTask.then([](int i) {
    debug("simple task end: ", i);
  }).catching([](std::exception &e) {
    debug("error occurred", e.what());
  });
  try {
    auto i = simpleTask.get_result();
    debug("simple task end from get: ", i);
  } catch (std::exception &e) {
    debug("error: ", e.what());
  }
}

int main() {
  test_tasks();

  auto looper = LooperExecutor();

  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1s);
  looper.shutdown(false);
  std::this_thread::sleep_for(1s);

  return 0;
}
