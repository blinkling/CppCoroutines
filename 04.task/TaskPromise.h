//
// Created by benny on 2022/3/17.
//

#ifndef CPPCOROUTINES_TASKS_04_TASK_TASKPROMISE_H_
#define CPPCOROUTINES_TASKS_04_TASK_TASKPROMISE_H_

#include <functional>
#include <mutex>
#include <list>
#include <optional>

#include "coroutine_common.h"
#include "Result.h"
#include "TaskAwaiter.h"

/*
promise_type 是与协程相关联的一个关键部分，它定义了协程的行为。promise_type需要实现某些必需的成员函数，而其他的则是可选的。

下面是promise_type可能需要的成员：

必需的成员：
- get_return_object():
返回与此协程关联的外部对象（通常是由用户直接使用的对象，例如 task 或 generator）。它是协程开始执行后第一个被调用的成员函数。

- initial_suspend():
返回一个awaitable，指示协程启动后是否应立即暂停。例如，返回 std::suspend_always 会导致协程立即暂停，而返回 std::suspend_never 会导致协程立即开始执行。

- final_suspend() noexcept:
返回一个awaitable，指示协程结束后是否应暂停。这通常用于确保外部对象（如 task 或 generator）保持活跃状态，直到协程完全结束。

- unhandled_exception():
用于捕获协程内部的异常。通常，你会在这里存储异常，并在协程恢复执行时重新抛出。

可选的成员：
- return_void():
用于void返回类型的协程。如果协程的返回类型是void，则必须实现此函数。

- return_value(T value):
用于非void返回类型的协程。如果你的协程计划返回某种类型（而不是void），则需要实现此函数。
这里的value来自哪儿呢?? 来自调用co_return value; 这里提供的值会传入 return_value(T value)

- yield_value(T value):
对于生成器类型的协程，此函数允许协程"产生"值，而不是返回一个单一的值。

- await_transform(T awaitable):
允许你转换或修改协程中使用的co_await表达式的结果。这可以用于注入自定义行为或修改awaitable对象。
*/


template<typename ResultType>
class Task;

template<typename ResultType>
struct TaskPromise {
  std::suspend_never initial_suspend() { return {}; }

  std::suspend_always final_suspend() noexcept { return {}; }

  Task<ResultType> get_return_object() {
    return Task{std::coroutine_handle<TaskPromise>::from_promise(*this)};
  }

  template<typename _ResultType>
  TaskAwaiter<_ResultType> await_transform(Task<_ResultType> &&task) {
    return TaskAwaiter<_ResultType>(std::move(task));
  }

  void unhandled_exception() {
    std::lock_guard lock(completion_lock);
    result = Result<ResultType>(std::current_exception());
    completion.notify_all();
    notify_callbacks();
  }

  void return_value(ResultType value) {
    std::lock_guard lock(completion_lock);
    result = Result<ResultType>(std::move(value));
    completion.notify_all();
    notify_callbacks();
  }

  ResultType get_result() {
    // blocking for result or throw on exception
    std::unique_lock lock(completion_lock);
    if (!result.has_value()) {
      completion.wait(lock);
    }
    return result->get_or_throw();
  }

  /*
  存储回调：在任务尚未完成时（即result还没有值时），如果有代码尝试注册一个完成回调，这个回调就会被添加到completion_callbacks列表中。
  */
  void on_completed(std::function<void(Result<ResultType>)> &&func) {
    std::unique_lock lock(completion_lock);
    if (result.has_value()) {
      auto value = result.value();
      lock.unlock();
      func(value);
    } else {
      completion_callbacks.push_back(func);
    }
  }

 private:
  std::optional<Result<ResultType>> result;

  std::mutex completion_lock;
  std::condition_variable completion;

  std::list<std::function<void(Result<ResultType>)>> completion_callbacks;

  void notify_callbacks() {
    auto value = result.value();
    for (auto &callback : completion_callbacks) {
      callback(value);
    }
    completion_callbacks.clear();
  }

};

#endif //CPPCOROUTINES_TASKS_04_TASK_TASKPROMISE_H_
