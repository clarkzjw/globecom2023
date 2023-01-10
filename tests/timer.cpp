#include <mutex>
#include <iostream>
#include <unistd.h>
#include "BS_thread_pool.hpp"
#include <cstdio>
#include <random>
#include <shared_mutex>
#include <map>
#include <date.h>
#include <chrono>
#include <string>
#include <sstream>
#include <matplot/matplot.h>
#include <cmath>

using namespace std;
using namespace std::chrono;

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

struct Test {
    int seg_no;
    int value;
};

std::string return_current_time_and_date() {
    auto now = time_point_cast<milliseconds>(system_clock::now());
    return date::format("%Y%m%d-%H%M%S", now);
}

int main() {
//    BS::thread_pool p(2);
//    for (int i = 0; i < 20; i++) {
//        p.push_task(schedule, i);
//    }
//
//    task_pool.wait_for_tasks();

//    map<int,Bar>::iterator it = m.find('2');
//    Bar b3;
//    if(it != m.end())
//    {
//        //element found;
//        b3 = it->second;
//    }


//    std::map<int, struct Test> test_map;
//
//    auto it = test_map.find(1);
//    if (test_map.find(1) != test_map.end()) {
//        printf("find\n");
//    } else {
//        printf("didn't fid\n");
//    }
//
//    test_map[1] = {1, 1};
//    test_map[2] = {2, 1};
//
//    it = test_map.find(1);
//    if (test_map.find(1) != test_map.end()) {
//        printf("find\n");
//    } else {
//        printf("didn't find\n");
//    }

//    cout << return_current_time_and_date() << endl;

    using namespace matplot;
    std::vector<double> x = linspace(0, 2 * pi);
    std::vector<double> y = transform(x, [](auto x) { return sin(x); });

    plot(x, y, "-o");
    hold(on);
    plot(x, transform(y, [](auto y) { return -y; }), "--xr");
    plot(x, transform(x, [](auto x) { return x / pi - 1.; }), "-:gs");
    plot({1.0, 0.7, 0.4, 0.0, -0.4, -0.7, -1}, "k");

    save("result.png", "png");
    return 0;
}