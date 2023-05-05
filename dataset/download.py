#! /usr/bin/env python3

"""
Modified from https://gist.github.com/Yang-Jace-Liu/f83a7328232f9934275beab79560f6ef
This script is to download Big Buck Bunny lossless 1080P version
The video is 1920x1080 24 FPS as a sequence of PNG files.

Video Source: https://media.xiph.org/BBB/BBB-1080-png/

Original Author: Yang Liu
Original Date: 2021-04-04
"""

import multiprocessing
from typing import Tuple, Optional

import argparse
import os
import requests

NUM_FRAMES = 14315
VIDEO_NAME = "Big Buck Bunny"
RES_STR = "1080P"
FPS = 24


def main():
    parser = argparse.ArgumentParser(description="A script to download %s lossless %s video" % (VIDEO_NAME, RES_STR))
    subparsers = parser.add_subparsers(help="sub-command", required=True, dest="command")

    parser_info = subparsers.add_parser("info", help="show info of the video")
    parser_info.set_defaults(func="info")

    parser_download = subparsers.add_parser("download", help="Download %s video" % VIDEO_NAME)
    parser_download.set_defaults(func="download")
    parser_download.add_argument("-s", "--start-frame", default=1, type=int,
                                 help="The frame number to start with, default is 1")
    parser_download.add_argument("-e", "--end-frame", default=14315, type=int,
                                 help="The frame number of the end, default is %d (the last frame)" % NUM_FRAMES)
    parser_download.add_argument("-o", "--output", required=True, type=str, help="The output folder")

    args = parser.parse_args()

    if args.func == "info":
        info(args)
    elif args.func == "download":
        download(args)


def info(args):
    print("Video Name: %s" % VIDEO_NAME)
    print("Number of frames: %d" % NUM_FRAMES)
    print("Resolution: %s" % RES_STR)
    print("FPS: %d" % FPS)


def download(args):
    start_frame = args.start_frame
    end_frame = args.end_frame
    output = args.output

    valid, msg = validate_args(args)
    if not valid:
        print(msg)
        return

    print("Downloading %s from frame %d to frame %d" % (VIDEO_NAME, start_frame, end_frame))
    download_frames(start_frame, end_frame, output)
    print("Download Complete")
    print()

    print("You can convert the png files to a continuous video by using following command: ")
    print("""ffmpeg -r 24 -i %s -c:v rawvideo -vf "fps=24,format=yuv420p" %s """ % (
        output + "/big_buck_bunny_%05d.png", output + "/bbb.yuv"))
    print()

    print("Then you can play the video by using the following command: ")
    print(
        """vlc --demux rawvideo --rawvid-fps %d --rawvid-width 1920 --rawvid-height 1080 --rawvid-chroma I420 %s/bbb.yuv""" % (
            FPS, output))


def validate_args(args) -> Tuple[bool, Optional[str]]:
    start_frame = args.start_frame
    end_frame = args.end_frame
    output = args.output

    if start_frame < 1 or start_frame > NUM_FRAMES:
        return False, "start_frame has to been in range [1-%d]" % NUM_FRAMES

    if end_frame < 1 or end_frame > NUM_FRAMES:
        return False, "end_frame has to been in range [1-%d]" % NUM_FRAMES

    if start_frame > end_frame:
        return False, "end_frame has to been greater than or equal to start_frame"

    if not os.path.isdir(output):
        return False, "output [%s] is not a valid folder" % output

    return True, None


def download_frames(start_frame, end_frame, output):
    frames = [i for i in range(start_frame, end_frame + 1)]
    with multiprocessing.Pool(processes=20) as pool:
        pool.starmap(download_frame, [(frame, output) for frame in frames], chunksize=1)
    print()


def download_frame(frame, output):
    url = "https://media.xiph.org/BBB/BBB-1080-png/big_buck_bunny_%05d.png" % frame
    output_file = "%s/big_buck_bunny_%05d.png" % (output, frame)
    print("\rDownloading frame %05d" % frame, end="")

    r = requests.get(url)
    with open(output_file, 'wb') as f:
        f.write(r.content)


if __name__ == '__main__':
    main()
