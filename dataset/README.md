# Create custom DASH dataset with BigBuckBunny raw video

## Prerequisites

+ ffmpeg
+ [Bento4](https://github.com/axiomatic-systems/Bento4)

## Download

Run `download.py` to download all raw frames in .png format and convert it to .yuv format

.yuv raw video can then be converted to .mp4 using ffmpeg

```bash
ffmpeg -f rawvideo -vcodec rawvideo -s 1920x1080 -r 24 -pix_fmt yuv420p -i bbb.yuv -c:v libx264 -preset slow -qp 0 -crf 0 bbb-1080p-24fps-raw.mp4
```

## Customization

Modify `resolutions`, `hw_map` and `crf_map` and then run `main.py` accordingly.

```python
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
```