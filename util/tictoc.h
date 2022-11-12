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
    typedef std::chrono::microseconds ms;
    clock::time_point p;

public:
    clock::time_point tic()
    {
        return clock::now();
    }

    double elapsed()
    {
        double elapsed = double(std::chrono::duration_cast<ms>(clock::now() - p).count()) / 1e6;
        std::cout << "Elapsed time: "
                  << elapsed
                  << " seconds." << std::endl;
        return elapsed;
    }
};

#endif // TPLAYER_TICTOC_H
