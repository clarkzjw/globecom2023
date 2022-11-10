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

IMPD*           DASHManager::Open   (char *path)
{
    std::string s_path = path;
    MPD* mpd;
    uint32_t fetchTime = Time::GetCurrentUTCTimeInSec();

    DOMParser parser(path);
    if (!parser.Parse()) return nullptr;
    mpd = parser.GetRootNode()->ToMPD();

    if (mpd)
        mpd->SetFetchTime(fetchTime);

    return mpd;
}
void            DASHManager::Delete ()
{
    delete this;
}
