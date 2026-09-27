#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <functional>
#include <chrono>

using namespace std;

// Represents a task with a priority
struct Task {
    int priority;
    function<void()> job;

    // Higher priority tasks come first
    bool operator<(const Task& other) const {
        return priority < other.priority;
    }
};

class TaskScheduler {
private:
    priority_queue<Task> taskQueue;
    vector<thread> workers;

    mutex queueMutex;
    condition_variable condition;

    bool stop = false;

    // Function executed by each worker thread
    void worker() {
        while (true) {
            Task task;

            {
                unique_lock<mutex> lock(queueMutex);

                // Wait until a task is available or scheduler is stopped
                condition.wait(lock, [this]() {
                    return stop || !taskQueue.empty();
                });

                // Exit when there are no more tasks
                if (stop && taskQueue.empty())
                    return;

                // Get the highest-priority task
                task = taskQueue.top();
                taskQueue.pop();
            }

            cout << "Thread " << this_thread::get_id()
                 << " executing task (Priority: "
                 << task.priority << ")" << endl;

            task.job();

            cout << "Thread " << this_thread::get_id()
                 << " completed task" << endl;
        }
    }

public:
    // Create the required number of worker threads
    TaskScheduler(int numberOfThreads) {
        for (int i = 0; i < numberOfThreads; i++) {
            workers.emplace_back(&TaskScheduler::worker, this);
        }
    }

    // Add a task to the scheduler
    void addTask(int priority, function<void()> job) {
        {
            lock_guard<mutex> lock(queueMutex);
            taskQueue.push({priority, job});
        }

        // Wake up a waiting worker
        condition.notify_one();
    }

    // Stop all worker threads
    ~TaskScheduler() {
        {
            lock_guard<mutex> lock(queueMutex);
            stop = true;
        }

        condition.notify_all();

        for (thread& worker : workers) {
            if (worker.joinable())
                worker.join();
        }
    }
};

int main() {

    // Create a scheduler with 3 worker threads
    TaskScheduler scheduler(3);

    // Add tasks with different priorities

    scheduler.addTask(1, []() {
        cout << "Task 1 is running..." << endl;
        this_thread::sleep_for(chrono::seconds(2));
    });

    scheduler.addTask(5, []() {
        cout << "Task 2 is running..." << endl;
        this_thread::sleep_for(chrono::seconds(1));
    });

    scheduler.addTask(3, []() {
        cout << "Task 3 is running..." << endl;
        this_thread::sleep_for(chrono::seconds(2));
    });

    scheduler.addTask(4, []() {
        cout << "Task 4 is running..." << endl;
        this_thread::sleep_for(chrono::seconds(1));
    });

    scheduler.addTask(2, []() {
        cout << "Task 5 is running..." << endl;
        this_thread::sleep_for(chrono::seconds(1));
    });

    // Allow enough time for all tasks to complete
    this_thread::sleep_for(chrono::seconds(5));

    return 0;
}
