//
// Created by Jinwei Zhao on 2022-09-06.
//
#include "tplayer.h"
#include <cassert>
using namespace std;

int exec_cmd(string cmd, string args)
{
    assert(system(string(cmd + " " + args).c_str()) == 0);
    return 0;
}
