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
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemSerial::sync() {
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

TaskSystemParallelSpawn::TaskSystemParallelSpawn(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
}

TaskSystemParallelSpawn::~TaskSystemParallelSpawn() {}

void TaskSystemParallelSpawn::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelSpawn::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                 const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelSpawn::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelSpawn in Part B.
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

TaskSystemParallelThreadPoolSpinning::TaskSystemParallelThreadPoolSpinning(int num_threads): ITaskSystem(num_threads) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
}

TaskSystemParallelThreadPoolSpinning::~TaskSystemParallelThreadPoolSpinning() {}

void TaskSystemParallelThreadPoolSpinning::run(IRunnable* runnable, int num_total_tasks) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }
}

TaskID TaskSystemParallelThreadPoolSpinning::runAsyncWithDeps(IRunnable* runnable, int num_total_tasks,
                                                              const std::vector<TaskID>& deps) {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    for (int i = 0; i < num_total_tasks; i++) {
        runnable->runTask(i, num_total_tasks);
    }

    return 0;
}

void TaskSystemParallelThreadPoolSpinning::sync() {
    // NOTE: CS149 students are not expected to implement TaskSystemParallelThreadPoolSpinning in Part B.
    return;
}

/*
 * ================================================================
 * Parallel Thread Pool Sleeping Task System Implementation
 * ================================================================
 */

// Constructor: Initializes the task system
TaskSystemParallelThreadPoolSleeping::TaskSystemParallelThreadPoolSleeping(int num_threads)
    : task_counter(0), unfinished_tasks(0), stop(false) {
    // Create worker threads
    for (int i = 0; i < num_threads; ++i) {
        workers.emplace_back(&TaskSystemParallelThreadPoolSleeping::workerThread, this);
    }
}

// Destructor: Shuts down the task system
TaskSystemParallelThreadPoolSleeping::~TaskSystemParallelThreadPoolSleeping() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all(); // Wake up all worker threads
    for (std::thread &worker : workers) {
        worker.join(); // Wait for each worker thread to finish
    }
}

// Main worker thread function
void TaskSystemParallelThreadPoolSleeping::workerThread() {
    while (true) {
        int task_id;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this] { return stop || !ready_queue.empty(); });

            if (stop && ready_queue.empty()) {
                return;
            }

            task_id = ready_queue.front();
            ready_queue.pop();
        }

        // Execute the task
        Task& task = tasks[task_id];
        task.func(); // Execute the task's function

        std::unique_lock<std::mutex> lock(queue_mutex);

        // Mark task as done and update dependencies
        for (int dependent_task_id : wait_map[task_id]) {
            Task& dependent_task = tasks[dependent_task_id];
            if (--dependent_task.dep_count == 0) {
                ready_queue.push(dependent_task_id);
                condition.notify_one(); // Wake up one thread to execute the dependent task
            }
        }

        // Clean up the task
        tasks.erase(task_id);
        unfinished_tasks--;

        // Notify if all tasks are done
        if (unfinished_tasks == 0) {
            all_tasks_done.notify_all();
        }
    }
}

// Submit a task with dependencies
TaskID TaskSystemParallelThreadPoolSleeping::runAsyncWithDeps(IRunnable* runnable, 
    int num_total_tasks, const std::vector<TaskID>& deps) {

    std::unique_lock<std::mutex> lock(queue_mutex);

    TaskID task_id = task_counter++;
    Task new_task;
    new_task.task_id = task_id;
    new_task.dep_count = deps.size(); // How many dependencies this task has

    //这个函数并不会在 runAsyncWithDeps 调用时立即执行，而是会在任务被调度到某个工作线程时才被执行
    new_task.func = [=]() {
        for (int i = 0; i < num_total_tasks; i++) {
            runnable->runTask(i, num_total_tasks);
        }
    };

    tasks[task_id] = new_task;
    unfinished_tasks++;

    // If no dependencies, add to ready queue immediately
    if (deps.empty()) {
        ready_queue.push(task_id);
        condition.notify_one(); // Wake up one thread
    } else {
        // Add to wait_map so it's ready when dependencies are done
        for (TaskID dep : deps) {
            wait_map[dep].push_back(task_id);
        }
    }

    return task_id;
}

// Synchronize (wait for all tasks to complete)
void TaskSystemParallelThreadPoolSleeping::sync() {
    std::unique_lock<std::mutex> lock(queue_mutex);
    all_tasks_done.wait(lock, [this] { return unfinished_tasks == 0; });
}
