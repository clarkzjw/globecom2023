import json
import time

from config import playback_buffer_map, rebuffering_ratio, history, playback_buffer_frame_count, \
    playback_buffer_frame_ratio


class MockPlayer:
    def __init__(self, exp_id: str):
        self.played_segments = 0
        self.downloaded_segments = 0
        self.ts_playback_start = time.time()
        self.rebuffering_event = list()
        self.experiment_id = exp_id

    def get_total_rebuffered_time(self):
        total = 0
        for x in self.rebuffering_event:
            if x["completed"] == 1:
                total += (x["end"] - x["start"])
            else:
                total += (time.time() - x["start"])
        return total

    # print statistics
    def print_statistics(self):
        print(history)
        t = str(int(time.time()))
        # write history to file with timestamp in filename
        if len(self.experiment_id) > 0:
            filename = "result/history_" + self.experiment_id + "_" + t + ".json"
        else:
            filename = "result/history_" + t + ".json"
        with open(filename, "w") as f:
            f.write(json.dumps(history._getvalue(), indent=4))
        print("history written to %s" % filename)

        print(self.rebuffering_event)
        if len(self.experiment_id) > 0:
            filename = "result/rebuffering_history_" + self.experiment_id + "_" + t + ".json"
        else:
            filename = "result/rebuffering_history_" + t + ".json"
        with open(filename, "w") as f:
            f.write(json.dumps(self.rebuffering_event, indent=4))

        print("history rebuffering event written to %s" % filename)

    def count_current_playable_frames(self, current_seg_no):
        playable_frames = 0
        keys = playback_buffer_map.keys()
        for i in keys:
            playable = True
            for j in range(current_seg_no + 1, i + 1):
                if j not in keys:
                    playable = False
                    break
            if playable:
                playable_frames += playback_buffer_map[i]["frame_count"]
        return playable_frames

    def play(self):
        while True:
            rebuffering_ratio.value = self.get_total_rebuffered_time() * 1.0 / (time.time() - self.ts_playback_start)
            playback_buffer_frame_count.value = self.count_current_playable_frames(self.played_segments + 1)
            playback_buffer_frame_ratio.value = playback_buffer_frame_count.value / 240

            if self.played_segments + 1 in playback_buffer_map.keys():
                # if the next segment is in the playback buffer, play it
                if len(self.rebuffering_event) > 0 and self.rebuffering_event[-1]["completed"] == 0:
                    self.rebuffering_event[-1]["end"] = time.time()
                    self.rebuffering_event[-1]["completed"] = 1

                task = playback_buffer_map[self.played_segments + 1]
                # if this is the last segment, stop the playback
                if task["eos"] == 1:
                    print("EOS")
                    break

                frames = task["frame_count"]

                print("playing segment ", self.played_segments + 1,
                      " with ", frames, " frames",
                      " current buffer size: ", len(playback_buffer_map),
                      sorted(playback_buffer_map.keys()))

                for i in range(int(frames)):
                    time.sleep(1.0 / 24)
                    playback_buffer_frame_count.value = (task["frame_count"] - i - 1) + self.count_current_playable_frames(self.played_segments + 1) * 1.0
                    playback_buffer_frame_ratio.value = playback_buffer_frame_count.value / 240

                self.played_segments += 1
                del playback_buffer_map[self.played_segments]
            else:
                if len(self.rebuffering_event) == 0 or self.rebuffering_event[-1]["completed"] == 1:
                    self.rebuffering_event.append({
                        "start": time.time(),
                        "end": -1,
                        "next_seg_no": self.played_segments + 1,
                        "completed": 0
                    })
                    print("######## New rebuffering event added at %f for seg %d" %
                          (self.rebuffering_event[-1]["start"],
                           self.rebuffering_event[-1]["next_seg_no"]))
                else:
                    # calculate rebuffering ratio
                    rebuffering_ratio.value = self.get_total_rebuffered_time() * 1.0 / (
                                time.time() - self.ts_playback_start)

        # end of playback
        self.rebuffering_event.append({
            "start": time.time(),
            "end": time.time(),
            "next_seg_no": -1,
            "completed": 1
        })
        self.print_statistics()
