//
// Created by clarkzjw on 11/11/22.
//
#include "tplayer.h"

/*
 * minRTT
 * */
void multipath_picoquic_minRTT()
{
    int nb_segments = (int)urls[0].size();
    int layers = (int)urls.size();

    TicToc global_timer;
    playback_start = global_timer.tic();

    std::thread thread_player(mock_player);

    quic_config->multipath_option = 2;
    quic_config->multipath_alt_config = (char*)malloc(sizeof(char) * (strlen(multipath_links) + 1));
    memset(quic_config->multipath_alt_config, 0, sizeof(char) * (strlen(multipath_links) + 1));
    memcpy(quic_config->multipath_alt_config, multipath_links, sizeof(char) * (strlen(multipath_links) + 1));
    printf("config.multipath_alt_config: %s\n", quic_config->multipath_alt_config);

    for (int i = 1; i < 100; i++) {
        while (player_buffer.size() > PLAYER_BUFFER_MAX_SEGMENTS) {
            PortableSleep(0.1);
        }

        string filename;
        PerSegmentStats pst;

        for (int j = 0; j < layers; j++) {
            filename += string("/1080/").append(urls[j][i].url);
            filename += ";";
        }
        printf("%s\n", filename.c_str());

        TicToc timer;

        pst.start = timer.tic();

        int seg_no = i;

        struct picoquic_download_stat picoquic_st { };

        int ret = quic_client(host.c_str(), port, quic_config, 0, 0, filename.c_str(), "", &picoquic_st);
        pst.end = timer.tic();

        printf("download ret = %d\n", ret);

        if (!seg_stats.contains(seg_no)) {
            for (int j = 0; j < layers; j++) {
                string tmp_filename = string("/1080/").append(urls[j][i].url);
                pst.file_size += get_filesize(tmp_filename);
            }
            pst.finished_layers = 4;
            PlayableSegment ps {};
            ps.nb_frames = 48;
            ps.eos = 0;
            ps.seg_no = seg_no;
            player_buffer.push(ps);

            seg_stats.insert({ seg_no, pst });
        }
    }
    PlayableSegment ps {};
    ps.eos = 1;
    player_buffer.push(ps);
    thread_player.join();

    std::string datafile = "minRTT.dat";
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
