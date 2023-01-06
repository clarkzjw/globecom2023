//
// Created by clarkzjw on 11/11/22.
//

#include "tplayer.h"

std::vector<BufferEvent> buffer_events_vec;

/*
 * Mock Player
 * */
// deprecated
void mock_player()
{
    double FPS = 24.0;

    TicToc buffering_timer;
    auto player_start = buffering_timer.tic();

    while (true) {
        if (!player_buffer.empty()) {
            if (!buffer_events_vec.empty() && buffer_events_vec[buffer_events_vec.size() - 1].completed == 0) {
                buffer_events_vec[buffer_events_vec.size() - 1].end = buffering_timer.tic();
                buffer_events_vec[buffer_events_vec.size() - 1].completed = 1;
            }
            struct PlayableSegment s = player_buffer.front();
            player_buffer.pop();

            // this is the last segment
            if (s.eos == 1) {
                break;
            }

            int frames = s.nb_frames;
            printf("Playing segment %d\n", s.seg_no);
            PortableSleep(1 / FPS * frames);
        } else {
            if (buffer_events_vec.empty() || buffer_events_vec[buffer_events_vec.size() - 1].completed == 1) {
                struct BufferEvent be { };
                be.start = buffering_timer.tic();

                buffer_events_vec.push_back(be);
                printf("######## New buffer_events_vec event added\n");
            } else {
                continue;
            }
        }
    }

    printf("\nbuffer_events_vec event metrics, buffer event count: %zu\n", buffer_events_vec.size());
    for (auto& be : buffer_events_vec) {
        std::cout << "buffer event, start: " << epoch_to_relative_seconds(player_start, be.start) << ", end: "
                  << epoch_to_relative_seconds(player_start, be.end) << endl;
    }
}

vector<SegmentPlaybackInfo> playback_info_vec;

void main_player_mock()
{
    double FPS = 24.0;

    TicToc buffering_timer;
    auto player_start = buffering_timer.tic();

    int nb_played_segments = 1;

    while (true) {
        // nb_played_segments acts as a pointer to the next segment to be played

        // if the next segment is in the buffer, play it
        if (player_buffer_map.find(nb_played_segments) != player_buffer_map.end())
        {
            // if buffer_events_vec is not empty and the previous last buffer event is not completed yet,
            // complete it now
            if (!buffer_events_vec.empty() && buffer_events_vec[buffer_events_vec.size() - 1].completed == 0) {
                buffer_events_vec[buffer_events_vec.size() - 1].end = buffering_timer.tic();
                buffer_events_vec[buffer_events_vec.size() - 1].completed = 1;
            }
            struct PlayableSegment s = player_buffer_map[nb_played_segments];

            // this is the last segment
            if (s.eos == 1) {
                break;
            }

            struct SegmentPlaybackInfo spi{};
            spi.seg_no = s.seg_no;
            spi.playback_duration = s.duration_seconds;
            spi.playback_start_second = epoch_to_relative_seconds(player_start, buffering_timer.tic());

            printf("Playing segment %d for %f seconds\n", s.seg_no, s.duration_seconds);
            PortableSleep(s.duration_seconds);
            spi.playback_end_second = epoch_to_relative_seconds(player_start, buffering_timer.tic());

            playback_info_vec.push_back(spi);
            nb_played_segments ++;
        } else {
            // if the next segment is not in the playback buffer, add a new buffer event
            if (buffer_events_vec.empty() || buffer_events_vec[buffer_events_vec.size() - 1].completed == 1) {
                struct BufferEvent be { };
                be.start = buffering_timer.tic();

                buffer_events_vec.push_back(be);
                printf("######## New buffer_events_vec event added\n");
            } else {
                continue;
            }
        }
    }

    printf("\nbuffer_events_vec event metrics, buffer event count: %zu\n", buffer_events_vec.size());
    for (auto& be : buffer_events_vec) {
        std::cout << "buffer event, start: " << epoch_to_relative_seconds(player_start, be.start) << ", end: "
                  << epoch_to_relative_seconds(player_start, be.end) << endl;
    }
}
