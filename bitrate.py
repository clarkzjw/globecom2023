from math import floor

from config import nb_paths

bitrate_mapping = {
    1080: {
        1: 87209.263,
        2: 71817.751,
        3: 55301.205,
    },
    720: {
        4: 35902.455,
        5: 22572.278,
    },
    360: {
        6: 4481.84,
    },
}

init_resolution = 1080


def get_resolution(bitrate_level) -> int:
    for key, value in bitrate_mapping.items():
        if bitrate_level in value:
            return key


def get_max_bitrate() -> float:
    return bitrate_mapping[1080][1]


def get_initial_bitrate() -> tuple[int, int, float]:
    bitrate = bitrate_mapping[init_resolution]
    count = 0
    for key, value in bitrate.items():
        if count == floor(len(bitrate) / 2):
            return init_resolution, key, value
        count += 1


def build_arms() -> list[int]:
    nb_arms = nb_paths * get_nb_bitrates()
    arms = []
    for i in range(nb_arms):
        arms.append(i + 1)

    return arms


def get_bitrate_level(resolution: int, bitrate: float) -> int:
    # get the bitrate level for the given resolution in bitrate_mapping
    for key, value in bitrate_mapping[resolution].items():
        if value == bitrate:
            return key


def get_nb_bitrates() -> int:
    nb_bitrates = 0
    for key, value in bitrate_mapping.items():
        nb_bitrates += len(value)
    return nb_bitrates
