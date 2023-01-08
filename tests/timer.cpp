#include <mutex>
#include <iostream>
#include <unistd.h>
#include "BS_thread_pool.hpp"
#include <cstdio>
#include <random>
using namespace std;

#define PortableSleep(seconds) usleep((seconds)*1000000)

std::mutex mutexes[2];

int get_random_int() {
    std::random_device rd;  // Use a random device to seed the generator
    std::mt19937 gen(rd());  // Standard mersenne_twister_engine
    std::uniform_int_distribution<> dis(1, 10);  // Generate random integers from 1 to 10

    return dis(gen);
}

void time_consuming_function(int var, int id) {
    int t = get_random_int();
    printf("path %d is selected for task %d, sleeping for %d\n", var, id, t);
    PortableSleep(t);
}

void task(int id) {
    int done = 0;
    while (true) {
        for (int i = 0; i < 2; i++) {
            if (mutexes[i].try_lock()) {
                time_consuming_function(i, id);
                mutexes[i].unlock();
                done = 1;
                break;
            }
        }
        if (done) {
            break;
        }
    }

}

int main() {
    BS::thread_pool task_pool(2);

    for (int i = 0; i < 10; i++) {
        task_pool.push_task(task, i);
//        PortableSleep(1);
    }

    return 0;
}