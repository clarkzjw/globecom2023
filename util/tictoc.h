//
// Created by Jinwei Zhao on 2022-09-08.
//

#ifndef TPLAYER_TICTOC_H
#define TPLAYER_TICTOC_H

#include <chrono>
#include <iostream>

class TicToc {
private:
    typedef std::chrono::system_clock clock;
    typedef std::chrono::microseconds res;
    clock::time_point t1, t2;

public:
    void tic()
    {
        t1 = clock::now();
    }

    double toc()
    {
        t2 = clock::now();
        double t = double(std::chrono::duration_cast<res>(t2 - t1).count()) / 1e6;
        std::cout << "Elapsed time: "
                  << t
                  << " seconds." << std::endl;
        return t;
    }
};

#endif // TPLAYER_TICTOC_H
