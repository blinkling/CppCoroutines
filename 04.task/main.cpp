//
// Created by benny on 2022/3/17.
//
#include "Task.h"
#include "io_utils.h"

/*
为何co_await 后面可以跟一个Task<int>? (通过simple_taskx返回)

一开始以为co_await后面需要跟的是一个等代替
也就是需要实现 await_suspend await_resume await_ready 的对象.
但这里, 之所以可以co_await后面接 Task<int>, 首先它是有 promise_type的

---插话
区别一个函数是不是协程，是通过它的返回值类型来判断的。如果它的返回值类型满足协程的规则，那这个函数就会被编译成协程。
那么，这个协程的规则是什么呢？规则就是返回值类型能够实例化下面的模板类型 _Coroutine_traits：

template <class _Ret, class = void>
struct _Coroutine_traits {};

//可以看出,这个返回体的内部, 需要定义一个promise_type的结构体, 这样才能保证这个trait满足条件.
template <class _Ret>
struct _Coroutine_traits<_Ret, void_t<typename _Ret::promise_type>> {
    using promise_type = typename _Ret::promise_type;
};

template <class _Ret, class...>
struct coroutine_traits : _Coroutine_traits<_Ret> {};
--- End

这里的Task就满足这一点, 
观察它的TaskPromise
里面有个重要的函数
  template<typename _ResultType>
  TaskAwaiter<_ResultType> await_transform(Task<_ResultType> &&task) {
    return TaskAwaiter<_ResultType>(std::move(task));
  }
它的作用, 就是返回一个等待体, 这个的作用就是, 限制了TaskAwaiter的行为, 不然co_await 可以接一个无关的等待体. 

所以这里的co_await 最终接到的是 TaskAwaiter<int>
这就明细了,co_await 依然接的是一个, 实现了 await_suspend await_resume await_ready 的对象.
然后就是它自己该有的逻辑了, 
如果await_ready返回false 就调用await_suspend
如果await_ready返回ture  就调用await_resume

问:
await_resume() 调用的机制
要不就是await_ready返回的是true就直接调用
要么就是通过 std::coroutine_handle<> handle去调用 resume 是吗??还有别的可能性吗

GPT:
你对co_await机制的理解是正确的。await_resume的调用时机取决于await_ready的返回值，以及协程框架如何处理await_suspend。
具体来说，await_resume的调用发生在以下两种情况之一：

如果await_ready返回true：这表示等待的操作已经完成，协程不需要挂起。在这种情况下，await_resume会立即被调用，协程继续执行。

如果await_ready返回false并且await_suspend被调用：在这种情况下，协程会被挂起。当等待的操作完成，并且某些外部事件（例如，
I/O操作完成、定时器超时、或其他协程释放了等待条件）导致协程需要被恢复时，会通过之前传递给await_suspend的
std::coroutine_handle<>对象调用resume()。在协程恢复执行后，await_resume会被调用。

其他可能性
await_suspend返回值的影响：await_suspend可以有不同的返回类型，这会影响协程的行为。例如，如果await_suspend返回void或
bool，那么协程会根据标准挂起逻辑进行挂起或继续执行。如果await_suspend返回一个std::coroutine_handle<>，则可以直接恢复
另一个协程，而不是恢复当前协程。这种情况下，await_resume的调用可能与直接由await_suspend返回的coroutine_handle的恢复
逻辑有关。

异常处理：如果在等待过程中发生异常（例如，异步操作失败），并且这个异常被捕获和传递（比如通过std::promise或类似机制），那么
await_resume的实现可以决定如何处理这个异常。它可能直接抛出异常，或者返回一个封装了错误状态的结果。

自定义逻辑：由于await_ready、await_suspend和await_resume是用户定义的函数，开发者可以在其中实现复杂的逻辑。例如，
await_resume可以执行一些额外的清理工作或状态检查，再返回结果。

总的来说，co_await表达式的行为非常灵活，可以通过实现这三个函数来适应各种异步编程需求。




*/

Task<int> simple_task2() {
  debug("task 2 start ...");
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1s);
  debug("task 2 returns after 1s.");
  co_return 2;
}

Task<int> simple_task3() {
  debug("in task 3 start ...");
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);
  debug("task 3 returns after 2s.");
  co_return 3;
}

Task<int> simple_task() {
  debug("task start ...");
  auto result2 = co_await simple_task2();
  debug("returns from task2: ", result2);
  auto result3 = co_await simple_task3();
  debug("returns from task3: ", result3);
  co_return 1 + result2 + result3;
}

int main() {
  auto simpleTask = simple_task();
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
  return 0;
}
