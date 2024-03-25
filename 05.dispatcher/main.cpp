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
Taskģ����ĵڶ�������, �ṩ��ִ������Ļ���(Executor)
������ϵķ�ʽ, ��Task�ڲ���promise_type����.

���������Ķ�ִ����:

�۲�promise_type�ĺ���:
template<typename _ResultType, typename _Executor>
TaskAwaiter<_ResultType, _Executor> await_transform(Task<_ResultType, _Executor> &&task) {
    return TaskAwaiter<_ResultType, _Executor>(&executor, std::move(task));
}

executor�ᱻ���ݸ�һ���ȴ���(DispatchAwaiter).
�ȴ����:
  void await_suspend(std::coroutine_handle<> handle) const {
    _executor->execute([handle]() {
      handle.resume();
    });
  }
����_executor����executeִ��������Ҫִ�е�����.

����������߼�:
ÿ�� co_await  һ��Task��ʱ��, ���Task�ͻ�ȥִ��.


ϸ�µ���һ��:
1. 
auto simpleTask = simple_task(); //��
���е�ʱ��, ֮ǰ��ѧ����, һ��Я���屻ִ�е�ʱ��, ��һ�����Ǵ���Я����, Ҳ��������� Task<int, LooperExecutor>
    ������ʱ���ȥ�������� get_return_object����
    - get_return_object():
    �������Э�̹������ⲿ����ͨ�������û�ֱ��ʹ�õĶ������� task �� generator��������Э�̿�ʼִ�к��һ�������õĳ�Ա������
    ��ɺ�ʼ���� initial_suspend , �۲������initial_suspend
    DispatchAwaiter initial_suspend() { return DispatchAwaiter{&executor}; }
    initial_suspend����DispatchAwaiter��ʱ��
    DispatchAwaiter��ʼ�������߼�:
    ���await_ready���ص���false, �Ǿ͹���ȥִ��await_suspend.
        void await_suspend(std::coroutine_handle<> handle) const {
        _executor->execute([handle]() {
        handle.resume();
          });
        }
      �������ȥִ��_executor->execute��ʱ��, execute�ᰴ�������߼�,ִ�д����lambda
      ���excute���첽��, ��ô��Ϊ�ڲ�������handle.resume(),handle.resume()����������await_resume()��������Ҫ����������������Լ����߼�, Ȼ��Я����ָ����˹����,��ʱ�Ѿ��л���
      �߳���ִ����. ��Ϊ��������µ��߳���resume��(executor��execute������).
        ���ʱ��Ĺ�������simple_task()���Я�����ʼ��λ��,(�Ҷ����������ʽ��,��Ϊ����ͨ��co_await)����ͨ��Я���崴��ʱ��Ļ���(�������۵�initial_suspend)
2. ����ǰ���ᵽ��resume���߳���, ����ִ�е��ˢ�:
   auto result2 = co_await simple_task2();//��
   �������ִ��Ҫ���µ�Я������, ���ظ���֮ǰ�Ĳ���:
   get_return_object���� Task<int, AsyncExecutor>
   ��������initial_suspend,
   �ж�DispatchAwaiter��await_ready , ok ����false ����:
   ִ��await_suspend, ע���������ε��õ���AsyncExecutor��, ��̬ûɶ��˵����, ִ������execute�߼�
   
   ʣ�µ�ûʲô˵����,��������߼�.


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
  auto result2 = co_await simple_task2();//��
  debug("returns from task2: ", result2);
  auto result3 = co_await simple_task3();
  debug("returns from task3: ", result3);
  co_return 1 + result2 + result3;
}

void test_tasks() {
  auto simpleTask = simple_task();//��
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
