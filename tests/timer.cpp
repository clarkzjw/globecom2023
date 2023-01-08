#include <mutex>
#include <iostream>
#include <unistd.h>
#include "BS_thread_pool.hpp"
#include <cstdio>
#include <random>
#include <shared_mutex>

using namespace std;

#define PortableSleep(seconds) usleep((seconds)*1000000)

std::mutex mutexes[2];

int get_random_int() {
    std::random_device rd;  // Use a random device to seed the generator
    std::mt19937 gen(rd());  // Standard mersenne_twister_engine
    std::uniform_int_distribution<> dis(1, 10);  // Generate random integers from 1 to 10

    return dis(gen);
}

void time_consuming_function(int var, int id, std::mutex *lock) {
    int t = get_random_int();
    printf("path %d is selected for task %d, sleeping for %d\n", var, id, t);
    PortableSleep(t);
}

std::shared_mutex previous_path_mutex;
int path_id;

int get_path_id() {
    std::shared_lock read_lock(previous_path_mutex);
    return path_id;
}

void task(int p_id, int id) {
    time_consuming_function(p_id, id, &mutexes[path_id]);
}

BS::thread_pool task_pool(2);

void schedule(int task_id) {
    std::unique_lock p_lock(previous_path_mutex);
    if (path_id == 0) {
        path_id = 1;
    } else if (path_id == 1){
        path_id = 0;
    }
    printf("task %d scheduled to path %d\n", task_id, path_id);
    task_pool.push_task(task, path_id, task_id);
}

int main() {
    BS::thread_pool p(2);
    for (int i = 0; i < 20; i++) {
        p.push_task(schedule, i);
    }

    task_pool.wait_for_tasks();
    return 0;
}