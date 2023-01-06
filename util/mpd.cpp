//
// Created by Jinwei Zhao on 2022-08-12.
//

#include "libdash.h"
#include "picoquic_config.h"
#include <iostream>

using namespace std;
using namespace dash::mpd;

dash::mpd::IMPD* parse_mpd(string& url, picoquic_quic_config_t* config)
{
    dash::IDASHManager* manager = CreateDashManager();
    return manager->Open((char*)url.c_str());
}

vector<vector<string>> get_segment_urls(dash::mpd::IMPD* mpd_file)
{
    auto periods = mpd_file->GetPeriods();
    auto adaption_sets = periods[0]->GetAdaptationSets();

    vector<vector<string>> urls;
    for (auto rep : adaption_sets[0]->GetRepresentation()) {
        vector<string> segment_urls;
        auto init_segment_url = rep->GetSegmentList()->GetInitialization()->GetSourceURL();
        segment_urls.push_back(init_segment_url);

        for (auto url : rep->GetSegmentList()->GetSegmentURLs()) {
            string u = url->GetMediaURI();
            segment_urls.push_back(u);
        }
        urls.push_back(segment_urls);
    }
    return urls;
}