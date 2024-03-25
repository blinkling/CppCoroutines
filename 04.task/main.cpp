//
// Created by benny on 2022/3/17.
//
#include "Task.h"
#include "io_utils.h"

/*
Ϊ��co_await ������Ը�һ��Task<int>? (ͨ��simple_taskx����)

һ��ʼ��Ϊco_await������Ҫ������һ���ȴ���
Ҳ������Ҫʵ�� await_suspend await_resume await_ready �Ķ���.
������, ֮���Կ���co_await����� Task<int>, ���������� promise_type��

---�廰
����һ�������ǲ���Э�̣���ͨ�����ķ���ֵ�������жϵġ�������ķ���ֵ��������Э�̵Ĺ�������������ͻᱻ�����Э�̡�
��ô�����Э�̵Ĺ�����ʲô�أ�������Ƿ���ֵ�����ܹ�ʵ���������ģ������ _Coroutine_traits��

template <class _Ret, class = void>
struct _Coroutine_traits {};

//���Կ���,�����������ڲ�, ��Ҫ����һ��promise_type�Ľṹ��, �������ܱ�֤���trait��������.
template <class _Ret>
struct _Coroutine_traits<_Ret, void_t<typename _Ret::promise_type>> {
    using promise_type = typename _Ret::promise_type;
};

template <class _Ret, class...>
struct coroutine_traits : _Coroutine_traits<_Ret> {};
--- End

�����Task��������һ��, 
�۲�����TaskPromise
�����и���Ҫ�ĺ���
  template<typename _ResultType>
  TaskAwaiter<_ResultType> await_transform(Task<_ResultType> &&task) {
    return TaskAwaiter<_ResultType>(std::move(task));
  }
��������, ���Ƿ���һ���ȴ���, ��������þ���, ������TaskAwaiter����Ϊ, ��Ȼco_await ���Խ�һ���޹صĵȴ���. 

���������co_await ���սӵ����� TaskAwaiter<int>
�����ϸ��,co_await ��Ȼ�ӵ���һ��, ʵ���� await_suspend await_resume await_ready �Ķ���.
Ȼ��������Լ����е��߼���, 
���await_ready����false �͵���await_suspend
���await_ready����ture  �͵���await_resume

��:
await_resume() ���õĻ���
Ҫ������await_ready���ص���true��ֱ�ӵ���
Ҫô����ͨ�� std::coroutine_handle<> handleȥ���� resume ����??���б�Ŀ�������

GPT:
���co_await���Ƶ��������ȷ�ġ�await_resume�ĵ���ʱ��ȡ����await_ready�ķ���ֵ���Լ�Э�̿����δ���await_suspend��
������˵��await_resume�ĵ��÷����������������֮һ��

���await_ready����true�����ʾ�ȴ��Ĳ����Ѿ���ɣ�Э�̲���Ҫ��������������£�await_resume�����������ã�Э�̼���ִ�С�

���await_ready����false����await_suspend�����ã�����������£�Э�̻ᱻ���𡣵��ȴ��Ĳ�����ɣ�����ĳЩ�ⲿ�¼������磬
I/O������ɡ���ʱ����ʱ��������Э���ͷ��˵ȴ�����������Э����Ҫ���ָ�ʱ����ͨ��֮ǰ���ݸ�await_suspend��
std::coroutine_handle<>�������resume()����Э�ָ̻�ִ�к�await_resume�ᱻ���á�

����������
await_suspend����ֵ��Ӱ�죺await_suspend�����в�ͬ�ķ������ͣ����Ӱ��Э�̵���Ϊ�����磬���await_suspend����void��
bool����ôЭ�̻���ݱ�׼�����߼����й�������ִ�С����await_suspend����һ��std::coroutine_handle<>�������ֱ�ӻָ�
��һ��Э�̣������ǻָ���ǰЭ�̡���������£�await_resume�ĵ��ÿ�����ֱ����await_suspend���ص�coroutine_handle�Ļָ�
�߼��йء�

�쳣��������ڵȴ������з����쳣�����磬�첽����ʧ�ܣ�����������쳣������ʹ��ݣ�����ͨ��std::promise�����ƻ��ƣ�����ô
await_resume��ʵ�ֿ��Ծ�����δ�������쳣��������ֱ���׳��쳣�����߷���һ����װ�˴���״̬�Ľ����

�Զ����߼�������await_ready��await_suspend��await_resume���û�����ĺ����������߿���������ʵ�ָ��ӵ��߼������磬
await_resume����ִ��һЩ�������������״̬��飬�ٷ��ؽ����

�ܵ���˵��co_await���ʽ����Ϊ�ǳ�������ͨ��ʵ����������������Ӧ�����첽�������




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
