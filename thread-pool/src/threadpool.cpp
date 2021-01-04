#include "threadpool.h"

thread_pool::thread_pool(size_t thread_count) {
  for (size_t i = 0; i < thread_count; ++i) {
    // start waiting threads. Workers listen for changes through
    //  the thread_pool member condition_variable
    _threads.emplace_back(std::thread([&]() {
      std::unique_lock<std::mutex> queue_lock(_task_mutex, std::defer_lock);

      while (true) {
        queue_lock.lock();
        _task_cv.wait(queue_lock, [&]() -> bool {
          return !_tasks.empty() || _stop_threads;
        });

        // used by dtor to stop all threads without having to
        //  unceremoniously stop tasks. The tasks must all be
        //  finished, lest we break a promise and risk a `future`
        //  object throwing an exception.
        if (_stop_threads && _tasks.empty())
          return;

        // to initialize temp_task, we must move the unique_ptr
        //  from the queue to the local stack. Since a unique_ptr
        //  cannot be copied (obviously), it must be explicitly
        //  moved. This transfers ownership of the pointed-to
        //  object to *this, as specified in 20.11.1.2.1
        //  [unique.ptr.single.ctor].
        auto temp_task = std::move(_tasks.front());

        _tasks.pop();
        queue_lock.unlock();

        (*temp_task)();
      }
    }));
  }
}

thread_pool::~thread_pool() {
  _stop_threads = true;
  _task_cv.notify_all();

  for (std::thread &thread : _threads) {
    thread.join();
  }
}
