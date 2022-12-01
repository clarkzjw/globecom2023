//
// Created by clarkzjw on 11/11/22.
//

#include "tplayer.h"

std::vector<BufferEvent> buffering;

/*
 * Mock Player
 * */
void mock_player()
{
    double FPS = 24.0;

    TicToc buffering_timer;
    auto player_start = buffering_timer.tic();

    while (true) {
        if (!player_buffer.empty()) {
            if (!buffering.empty() && buffering[buffering.size() - 1].completed == 0) {
                buffering[buffering.size() - 1].end = buffering_timer.tic();
                buffering[buffering.size() - 1].completed = 1;
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
            if (buffering.empty() || buffering[buffering.size() - 1].completed == 1) {
                struct BufferEvent be { };
                be.start = buffering_timer.tic();

                buffering.push_back(be);
                printf("######## New buffering event added\n");
            } else {
                continue;
            }
        }
    }

    printf("\nbuffering event metrics, buffer event count: %zu\n", buffering.size());
    for (auto& be : buffering) {
        std::cout << "buffer event, start: " << epoch_to_relative_seconds(player_start, be.start) << ", end: "
                  << epoch_to_relative_seconds(player_start, be.end) << endl;
    }
}


void mock_player_hash_map()
{
    double FPS = 24.0;

    TicToc buffering_timer;
    auto player_start = buffering_timer.tic();

    int nb_played_segments = 1;
    while (true) {
        if (player_buffer_map.find(nb_played_segments) != player_buffer_map.end())
        {
            std::cout << "player buffer contains " << nb_played_segments << std::endl;

            if (!buffering.empty() && buffering[buffering.size() - 1].completed == 0) {
                buffering[buffering.size() - 1].end = buffering_timer.tic();
                buffering[buffering.size() - 1].completed = 1;
            }
            struct PlayableSegment s = player_buffer_map[nb_played_segments];

            // this is the last segment
            if (s.eos == 1) {
                break;
            }

            int frames = s.nb_frames;
            printf("Playing segment %d\n", s.seg_no);
            PortableSleep(1 / FPS * frames);
            nb_played_segments ++;

        } else {
//            std::cout << "segment " << nb_played_segments << " not ready in player buffer yet" << endl;
            if (buffering.empty() || buffering[buffering.size() - 1].completed == 1) {
                struct BufferEvent be { };
                be.start = buffering_timer.tic();

                buffering.push_back(be);
                printf("######## New buffering event added\n");
            } else {
                continue;
            }
        }
    }

    printf("\nbuffering event metrics, buffer event count: %zu\n", buffering.size());
    for (auto& be : buffering) {
        std::cout << "buffer event, start: " << epoch_to_relative_seconds(player_start, be.start) << ", end: "
                  << epoch_to_relative_seconds(player_start, be.end) << endl;
    }
}
