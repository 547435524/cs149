#ifndef _TASKSYS_H
#define _TASKSYS_H

#include "itasksys.h"
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <atomic>
/*
 * TaskSystemSerial: This class is the student's implementation of a
 * serial task execution engine.  See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemSerial: public ITaskSystem {
    public:
        TaskSystemSerial(int num_threads);
        ~TaskSystemSerial();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
};

/*
 * TaskSystemParallelSpawn: This class is the student's implementation of a
 * parallel task execution engine that spawns threads in every run()
 * call.  See definition of ITaskSystem in itasksys.h for documentation
 * of the ITaskSystem interface.
 */
class TaskSystemParallelSpawn: public ITaskSystem {
    public:
        TaskSystemParallelSpawn(int num_threads);
        ~TaskSystemParallelSpawn();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
    private:
        void workerThread();  // 工作线程的主函数
        std::vector<std::thread> workers;           // 工作线程池
        std::queue<std::function<void()>> tasks;    // 任务队列
        std::mutex queue_mutex;                     // 队列的互斥锁
        std::condition_variable condition;          // 通知工作线程的条件变量
        std::atomic<bool> stop;                     // 线程池停止标志
        std::atomic<int> unfinished_tasks;          // 未完成任务的计数器
        std::condition_variable all_tasks_done;     // 用于等待所有任务完成
};

/*
 * TaskSystemParallelThreadPoolSpinning: This class is the student's
 * implementation of a parallel task execution engine that uses a
 * thread pool. See definition of ITaskSystem in itasksys.h for
 * documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSpinning: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSpinning(int num_threads);
        ~TaskSystemParallelThreadPoolSpinning();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
    private:
        void workerThread();  // 工作线程的主函数
        std::vector<std::thread> workers;           // 工作线程池
        std::queue<std::function<void()>> tasks;    // 任务队列
        std::mutex queue_mutex;                     // 队列的互斥锁
        std::condition_variable condition;          // 通知工作线程的条件变量
        std::atomic<bool> stop;                     // 线程池停止标志
        std::atomic<int> unfinished_tasks;          // 未完成任务的计数器
        std::condition_variable all_tasks_done;     // 用于等待所有任务完成


};

/*
 * TaskSystemParallelThreadPoolSleeping: This class is the student's
 * optimized implementation of a parallel task execution engine that uses
 * a thread pool. See definition of ITaskSystem in
 * itasksys.h for documentation of the ITaskSystem interface.
 */
class TaskSystemParallelThreadPoolSleeping: public ITaskSystem {
    public:
        TaskSystemParallelThreadPoolSleeping(int num_threads);
        ~TaskSystemParallelThreadPoolSleeping();
        const char* name();
        void run(IRunnable* runnable, int num_total_tasks);
        TaskID runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                const std::vector<TaskID>& deps);
        void sync();
    private:
        void workerThread();  // 工作线程的主函数
        std::vector<std::thread> workers;           // 工作线程池
        std::queue<std::function<void()>> tasks;    // 任务队列
        std::mutex queue_mutex;                     // 队列的互斥锁
        std::condition_variable condition;          // 通知工作线程的条件变量
        std::atomic<bool> stop;                     // 线程池停止标志
        std::atomic<int> unfinished_tasks;          // 未完成任务的计数器
        std::condition_variable all_tasks_done;     // 用于等待所有任务完成
};

#endif
