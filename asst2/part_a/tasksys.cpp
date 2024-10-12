#include "tasksys.h"


IRunnable::~IRunnable() {}

ITaskSystem::ITaskSystem(int num_threads) {}
ITaskSystem::~ITaskSystem() {}

/*
 * ================================================================
 * Serial task system implementation
 * ================================================================
 */

const char* TaskSystemSerial::name() {
    return "Serial";
}

TaskSystemSerial::TaskSystemSerial(int num_threads): ITaskSystem(num_threads) {
}

TaskSystemSerial::~TaskSystemSerial() {}

void TaskSystemSerial::run(IRunnable* runnable, int num_total_tasks) {
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemSerial::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                          const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemSerial::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelSpawn::name() {
    return "Parallel + Always Spawn";
}

// 构造函数
TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads)
    : ITaskSystem(num_threads), stop(false), unfinished_tasks(0) {

}


// 析构函数
TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {

}


// run 方法：并行执行任务
void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // for (int i = 0; i < num_total_tasks; i++) {
    //     runnable->runTask(i, num_total_tasks);
    // }

    // Vector to hold the temporary threads
    std::vector<std::thread> task_threads;

    // 启动多个任务线程
    for (int i = 0; i < num_total_tasks; ++i) {
        task_threads.emplace_back([=]() {
            runnable->runTask(i, num_total_tasks);
        });
    }

    // 等待所有任务线程完成
    for (std::thread& t : task_threads) {
        if (t.joinable()) {
            t.join(); // 等待每个任务线程完成
        }
    }
}



TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Spinning Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSpinning::name() {
    return "Parallel + Thread Pool + Spin";
}

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads)
    :ITaskSystem(num_threads), stop(false), unfinished_tasks(0) {    

    // 初始化线程池
    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back(&TaskSystemParallelThreadPoolSpinning::workerThread, this);
    }
}

void TaskSystemParallelThreadPoolSpinning:: workerThread() {
    while (!stop) {
        std::function<void()> task;
        if (!tasks.empty()) {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (!tasks.empty()) {
                task = std::move(tasks.front());
                tasks.pop();
            }
        }
        if (task) {
            task();
            --unfinished_tasks;// 原子变量，不用加锁
        }
    }
}


TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {
    stop = true;
    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {

    // 将任务加入队列
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        unfinished_tasks += num_total_tasks;
        for (int i = 0; i < num_total_tasks; ++i) {
            tasks.push([=] { runnable->runTask(i, num_total_tasks); });
        }
    }

    // 等待所有任务完成
    while (unfinished_tasks > 0) {
        // 主线程等待所有任务完成
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // You do not need to implement this method.
    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // You do not need to implement this method.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

const char* TaskSystemParallelThreadPoolSleeping::name() {
    return "Parallel + Thread Pool + Sleep";
}

TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads)
: ITaskSystem(num_threads) , stop(false), unfinished_tasks(0){
    // 初始化线程池
    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back(&TaskSystemParallelThreadPoolSleeping::workerThread, this);
    }

}

TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {

    stop = true;
    condition.notify_all(); // 唤醒所有等待的线程
    for (std::thread &worker : workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
}

void TaskSystemParallelThreadPoolSleeping::run(IRunnable* runnable, int num_total_tasks) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        unfinished_tasks += num_total_tasks;
        for (int i = 0; i < num_total_tasks; ++i) {
            tasks.push([=] { runnable->runTask(i, num_total_tasks); });
        }
    }

    // 通知所有工作线程有任务
    condition.notify_all();

    // 等待所有任务完成
    std::unique_lock<std::mutex> lock(queue_mutex);
    all_tasks_done.wait(lock, [this] { return unfinished_tasks == 0; });
}

 void TaskSystemParallelThreadPoolSleeping:: workerThread() {
    while (true) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return !tasks.empty() || stop; });
            if (stop && tasks.empty()) {
                return;
            }
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            --unfinished_tasks;
            if (unfinished_tasks == 0) {
                all_tasks_done.notify_one();
            }
        }
    }
 }

TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                    const std::vector<TaskID>& deps) {


    //
    // TODO: CS149 students will implement this method in Part B.
    //

    return 0;
}

void TaskSystemParallelThreadPoolSleeping::sync() {

    //
    // TODO: CS149 students will modify the implementation of this method in Part B.
    //

    return;
}
