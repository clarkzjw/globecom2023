//
// Created by Jinwei Zhao on 2022-08-12.
//

#include "tplayer.h"
#include <iostream>
#include <regex>

using namespace std;
using namespace dash::mpd;

dash::mpd::IMPD* parse_mpd(string& url)
{
    dash::IDASHManager* manager = CreateDashManager();
    return manager->Open((char*)url.c_str());
}

vector<vector<struct SegmentInfo>> get_segment_urls(dash::mpd::IMPD* mpd_file)
{
    auto periods = mpd_file->GetPeriods();
    auto adaption_sets = periods[0]->GetAdaptationSets();

    vector<vector<struct SegmentInfo>> temp_urls;
    vector<string> representation_ids;
    for (auto adaption_set : adaption_sets) {
        auto representations = adaption_set->GetRepresentation();
        for (auto representation : representations) {
            representation_ids.push_back(representation->GetId());
        }
    }
    // if AdaptationSet has SegmentTemplate
    for (auto adaption_set : adaption_sets) {
        // loop through representation_ids
        for (const auto& representation_id : representation_ids) {
            const ISegmentTemplate* segment_template = adaption_set->GetSegmentTemplate();
            if (segment_template) {
                string media_url_template = segment_template->Getmedia();
                string init_url_template = segment_template->Getinitialization();

                const ISegmentTimeline* segment_timeline = segment_template->GetSegmentTimeline();
                auto timescale = segment_template->GetTimescale();

                if (segment_timeline) {
                    auto segments = segment_timeline->GetTimelines();
                    vector<struct SegmentInfo> url;

                    init_url_template = regex_replace(init_url_template, regex("\\$RepresentationID\\$"), representation_id);
                    url.push_back({init_url_template, 0, 0});

                    int seg_no = 1;
                    for (const ITimeline* segment : segments) {
                        auto duration = segment->GetDuration();
                        auto repeat = segment->GetRepeatCount();

                        for (int i = 0; i < repeat + 1; i++) {
                            struct SegmentInfo segment_info;

                            // https://www.brendanlong.com/the-structure-of-an-mpeg-dash-mpd.html
                            segment_info.duration_seconds = (double)duration / (double)timescale;

                            segment_info.duration = duration;
                            // replace $RepresentationID$ in media_url_template with representation_id
                            segment_info.url = regex_replace(media_url_template, regex("\\$RepresentationID\\$"), representation_id);
                            // replace $Number$ in media_url_template with seg_no
                            segment_info.url = regex_replace(segment_info.url, regex("\\$Number\\$"), to_string(seg_no));
                            url.push_back(segment_info);
                            seg_no++;
                        }
                    }
                    temp_urls.push_back(url);
                }
            }
        }
    }

#if TPLAYER_DEBUG
    // loop through temp_urls and print them
    for (auto& u : temp_urls) {
        for (auto& url : u) {
            if (url.url.find("init") != string::npos) {
                cout << url.url << endl;
            } else {
                cout << url.url << " " << url.duration << " " << url.duration_seconds << endl;
            }
        }
    }
#endif

// deprecated old mpd code
//    for (auto rep : adaption_sets[0]->GetRepresentation()) {
//        vector<string> segment_urls;
//        auto init_segment_url = rep->GetSegmentList()->GetInitialization()->GetSourceURL();
//        segment_urls.push_back(init_segment_url);
//
//        for (auto url : rep->GetSegmentList()->GetSegmentURLs()) {
//            string u = url->GetMediaURI();
//            segment_urls.push_back(u);
//        }
//        urls.push_back(segment_urls);
//    }
    return temp_urls;
}