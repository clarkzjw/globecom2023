import os
import subprocess

resolutions = [
    "1080p",
    "720p",
    "480p",
    "360p",
    "240p",
    "144p"
]

hw_map = {
    "1080p": "1920x1080",
    "720p": "1280x720",
    "480p": "854x480",
    "360p": "640x360",
    "240p": "426x240",
    "144p": "256x144",
}

crf_map = {
    "1080p": [1, 16, 2],
    "720p": [1, 16, 2],
    "480p": [1, 16, 2],
    "360p": [1, 16, 2],
    "240p": [1, 16, 2],
    "144p": [1, 16, 2],
}

ffmpeg_profiles = ["main"]
fps = "24"

raw = "bbb-1080p-24fps-raw.mp4"
codec = "-c:v libx264"


def generate_crf():
    for r in resolutions:
        if not os.path.exists(r):
            continue

        r_raw = "bbb-{resolution}-{fps}fps-raw.mp4".format(resolution=r, fps=fps)
        for crf in range(crf_map[r][0], crf_map[r][1], crf_map[r][2]):
            hw = hw_map[r]

            for profile in ffmpeg_profiles:
                output_filename = "{resolution}/bbb-{resolution}-{fps}fps-{profile}-crf-{crf}.mp4". \
                    format(resolution=r, fps=fps, profile=profile, crf=crf)

                cmd = [
                    "ffmpeg", "-i", r_raw,
                    "-c:v", "libx264",
                    "-crf", str(crf),
                    "-profile:v", profile,
                    "-r", fps,
                    "-s", hw,
                    "-force_key_frames", "expr:eq(mod(n,{}),0)".format(int(fps) * 2),
                    "-x264opts", "rc-lookahead={}:keyint={}:min-keyint={}".format(int(fps) * 2, int(fps) * 4, int(fps) * 2),
                    output_filename
                ]

                result = subprocess.run(cmd, stdout=subprocess.PIPE)
                print(result.stdout.decode("utf-8").strip())


def query_bitrates(profile="main"):
    for r in resolutions:
        if not os.path.exists(r):
            continue
        print(r)
        for crf in range(crf_map[r][0], crf_map[r][1], crf_map[r][2]):
            filename = "{resolution}/bbb-{resolution}-{fps}fps-{profile}-crf-{crf}.mp4".format(
                resolution=r, fps=fps, profile=profile, crf=crf)

            cmd = ["ffprobe",
                   "-v", "error",
                   "-select_streams",
                   "v:0",
                   "-show_entries",
                   "stream=bit_rate",
                   "-of",
                   "default=noprint_wrappers=1:nokey=1",
                   filename]
            bitrate = subprocess.run(cmd, stdout=subprocess.PIPE).stdout.decode("utf-8").strip()
            # print("crf {}, bitrate {} Kbps".format(crf, float(bitrate) / 1000))
            print(float(bitrate) / 1000)


def fragment_videos():
    for r in resolutions:
        for crf in range(crf_map[r][0], crf_map[r][1], crf_map[r][2]):
            for profile in ffmpeg_profiles:
                filename = "{resolution}/bbb-{resolution}-{fps}fps-{profile}-crf-{crf}.mp4".format(
                    resolution=r, fps=fps, profile=profile, crf=crf)
                output_filename = "{resolution}-fragmented/bbb-{resolution}-{fps}fps-{profile}-crf-{crf}.mp4".format(
                    resolution=r, fps=fps, profile=profile, crf=crf)
                cmd = ["mp4fragment", filename, output_filename]
                result = subprocess.run(cmd, stdout=subprocess.PIPE)
                print(result.stdout.decode("utf-8").strip())


def create_dash_mpd(profile="main"):
    filename_list = []
    for r in resolutions:
        for crf in range(crf_map[r][0], crf_map[r][1], crf_map[r][2]):
            filename = "{resolution}-fragmented/bbb-{resolution}-{fps}fps-{profile}-crf-{crf}.mp4".format(
                resolution=r, fps=fps, profile=profile, crf=crf)
            filename_list.append(filename)
    cmd = ["mp4dash",
           "--use-segment-timeline",
           ] + filename_list + ["-o", "mpd"]

    result = subprocess.run(cmd, stdout=subprocess.PIPE)
    print(result.stdout.decode("utf-8").strip())


def scale_raw_videos():
    for r in resolutions:
        if r == "1080p":
            continue
        output_filename = "bbb-{resolution}-{fps}fps-raw.mp4".format(resolution=r, fps=fps)
        print(output_filename)
        cmd = [
            "ffmpeg", "-i", raw,
            "-vf", "scale={}".format(r.replace("x", ":")),
            "-c:v", "libx264",
            "-crf", "0",
            "-preset", "veryslow",
            "-c:a", "copy",
            output_filename
        ]
        result = subprocess.run(cmd, stdout=subprocess.PIPE)
        print(result.stdout.decode("utf-8").strip())


if __name__ == '__main__':
    scale_raw_videos()
    generate_crf()
    query_bitrates()
    fragment_videos()
    create_dash_mpd()
