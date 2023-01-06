//
// Created by clarkzjw on 1/5/23.
//

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <mutex>
#include <queue>
#include "tplayer.h"
#include <vector>
#include <string>
#include <regex>

using namespace std;

int get_filesize(const string& req_filename)
{
    std::string default_dir = "./tmp/";
    std::string a = req_filename.substr(1);
    std::replace(a.begin(), a.end(), '/', '_');
    std::string path = default_dir + a;

    std::filesystem::path p { path };
    size_t realsize = std::filesystem::file_size(p);
    std::cout << "filesize: " << realsize << std::endl;
    return (int)realsize;
}

double epoch_to_relative_seconds(tic_clock::time_point start, tic_clock::time_point end)
{
    return double(std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()) / 1e6;
}
