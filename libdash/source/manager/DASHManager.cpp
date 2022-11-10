/*
 * DASHManager.cpp
 *****************************************************************************
 * Copyright (C) 2012, bitmovin Softwareentwicklung OG, All Rights Reserved
 *
 * Email: libdash-dev@vicky.bitmovin.net
 *
 * @contributor        Daniele Lorenzi
 * @contributoremail   lorenzidaniele.97@gmail.com
 * @contributiondate   2021
 * 
 * This source code and its use and distribution, is subject to the terms
 * and conditions of the applicable license agreement.
 *****************************************************************************/

#include <algorithm>
#include "DASHManager.h"
#include "../network/picoquic.h"
#include "../network/picoquic_sample.h"

using namespace dash;
using namespace dash::xml;
using namespace dash::mpd;
using namespace dash::network;
using namespace dash::helpers;

DASHManager::DASHManager            ()
{
}
DASHManager::~DASHManager           ()
{
}

std::string           DASHManager::OpenQUIC   (char *path)
{
    std::string default_download_dir = "/tmp/";

    picoquic_quic_config_t config;
    picoquic_config_init(&config);
    config.out_dir = default_download_dir.c_str();

    // example path
    // quic://localhost:4433/1080/BBB-I-1080p.mpd
    std::string str_path = std::string(path);

    std::string uri = str_path.substr (str_path.find("quic://")+7);
    std::string filename = uri.substr(uri.find('/'));

    std::string hostport = uri.substr(0, uri.find('/'));
    std::string host = hostport.substr(0, hostport.find(':'));
    int port = std::stoi(hostport.substr(hostport.find(':')+1));

    int ret = quic_client(host.c_str(), port, &config, filename.c_str());
    printf("Download result: %d\n", ret);

    std::string localpath = filename.substr(1);
    std::replace( localpath.begin(), localpath.end(), '/', '_');

    return default_download_dir + localpath;
}

IMPD*           DASHManager::Open   (char *path)
{
    std::string s_path = path;
    MPD* mpd;
    uint32_t fetchTime = Time::GetCurrentUTCTimeInSec();

    if (s_path.rfind("quic://", 0) == 0) {
        DOMParser parser(this->OpenQUIC(path));
        if (!parser.Parse()) return nullptr;
        mpd = parser.GetRootNode()->ToMPD();
    } else {
        DOMParser parser(path);
        if (!parser.Parse()) return nullptr;
        mpd = parser.GetRootNode()->ToMPD();
    }

    if (mpd)
        mpd->SetFetchTime(fetchTime);

    return mpd;
}
void            DASHManager::Delete ()
{
    delete this;
}
