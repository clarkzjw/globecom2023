//
// Created by clarkzjw on 11/11/22.
//

#include "tplayer.h"

#if 0
/*
 * Round Robin
 * */
extern int nb_segments;

// create a downloader for each path
// each path poll available tasks at the same time
void path_downloader(int path_index)
{
    while (true) {
        if (!tasks.empty()) {
            while (player_buffer.size() > PLAYER_BUFFER_MAX_SEGMENTS) {
                PortableSleep(0.1);
            }
            struct DownloadTask t = tasks.front();
            tasks.pop();

            if (t.eos == 1) {
                printf("path %d ends\n", path_index);
                break;
            } else {
                int ret = 0;
                string if_name = path_ifname_vec[path_index];
                printf("\n\ndownloading %s on path %s\n", t.filename.c_str(), if_name.c_str());

                struct DownloadStats st;
                st.filename = t.filename;
                st.seg_no = t.seg_no;

                struct picoquic_download_stat picoquic_st { };

                PerSegmentStats pst;
                TicToc timer;

                pst.start = timer.tic();

                ret = quic_client(host.c_str(), port, quic_config, 0, 0, t.filename.c_str(), if_name.c_str(), &picoquic_st);

                pst.finished_layers = 1;

                // if this segment is shown for the first time
                if (!seg_stats.contains(t.seg_no)) {
                    pst.file_size = get_filesize(t.filename);
                    seg_stats.insert({ t.seg_no, pst });
                } else {
                    seg_stats[t.seg_no].end = timer.tic();
                    seg_stats[t.seg_no].finished_layers++;
                    seg_stats[t.seg_no].file_size += get_filesize(t.filename);

                    if (seg_stats[t.seg_no].finished_layers == 4) {
                        PlayableSegment ps {};
                        ps.nb_frames = 48;
                        ps.eos = 0;
                        ps.seg_no = t.seg_no;
//                        player_buffer.push(ps);
                        player_buffer_map[ps.seg_no] = ps;
                    }
                }

                st.time = timer.elapsed();
                st.speed = picoquic_st.throughput;
                st.path_index = path_index;
                st.filesize = get_filesize(t.filename);
                st.actual_speed = st.filesize / st.time / 1000000.0 * 8.0;

                stats.push_back(st);

                printf("download ret = %d\n", ret);
            }
        }
    }
}

void multipath_round_robin_download_queue()
{
    std::thread t1(path_downloader, 0);
    std::thread t2(path_downloader, 1);

    t1.join();
    t2.join();

    PlayableSegment ps {};
    ps.eos = 1;
    player_buffer_map[nb_segments] = ps;
}

// we need a queue to store the tasks
void multipath_round_robin()
{
    int layers = (int)urls.size();

    TicToc global_timer;
    playback_start = global_timer.tic();

    std::thread thread_download(multipath_round_robin_download_queue);
    std::thread thread_player(mock_player);

    for (int i = 1; i < nb_segments; i++) {
        for (int j = 0; j < layers; j++) {
            string filename = string("/1080/").append(urls[j][i].url);

            struct DownloadTask t;
            t.filename = filename;
            t.seg_no = i;
            t.eos = 0;

            tasks.push(t);
            printf("pushed %s\n", filename.c_str());
        }
    }

    struct DownloadTask finish;
    finish.eos = 1;
    tasks.push(finish);
    tasks.push(finish);
    thread_download.join();
    thread_player.join();

    printf("\nper layer download metrics, nb_layers downloaded: %zu\n", stats.size());
    for (auto& stat : stats) {
        printf("seg_no: %d, filename: %s, filesize: %d, time: %f, speed: %f, actual_speed: %f, path_index: %d\n", stat.seg_no, stat.filename.c_str(), stat.filesize, stat.time, stat.speed, stat.actual_speed, stat.path_index);
    }

    std::string datafile = "rr.dat";
    ChStreamOutAsciiFile mdatafile(datafile.c_str());

    printf("\nper segment download metrics\n");
    for (auto& [seg_no, value] : seg_stats) {
        value.download_time = double(std::chrono::duration_cast<std::chrono::microseconds>(value.end - value.start).count()) / 1e6;
        value.download_speed = value.file_size / value.download_time / 1000000.0 * 8.0;

        mdatafile << seg_no << ", " << value.download_speed << "\n";

        std::cout << seg_no << " = "
                  << "used: " << value.download_time
                  << " seconds, "
                  << "ends at " << epoch_to_relative_seconds(playback_start, value.end)
                  << " speed: " << value.download_speed << " Mbps"
                  << " total filesize " << value.file_size << " bytes" << endl;
    }
}
#endif
