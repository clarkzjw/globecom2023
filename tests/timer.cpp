//
// Created by clarkzjw on 1/6/23.
//

#include "util/tictoc.h"
#include <unistd.h>

using namespace std;

#define PortableSleep(seconds) usleep((seconds)*1000000)

int main() {
//    TicToc timer;
//    auto start = timer.tic();
//    PortableSleep(5);
//    auto end = timer.tic();
//
//    cout << "time elapsed: " << epoch_to_relative_seconds(start, end) << endl;
//    return 0;


    auto start = Tic();
    PortableSleep(5);
    auto end = Tic();

    cout << "time elapsed: " << epoch_to_relative_seconds(start, end) << endl;
    return 0;
}
