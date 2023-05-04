from multiprocessing import Manager

max_playback_buffer_size = 10
nb_paths = 2

manager = Manager()
playback_buffer_map = Manager().dict()

rebuffering_ratio = Manager().Value('d', 0.0)
playback_buffer_frame_ratio = Manager().Value('d', 0.0)
playback_buffer_frame_count = Manager().Value('d', 0.0)

history = Manager().list()

exp_id = ""
algorithm = ""
linucb_alpha = 0.1
lints_alpha = 0.1
egreedy_epsilon = 0.1
DEBUG_SEGMENTS = 200


def init(_exp_id: str, _algorithm: str, _linucb_alpha: float, _lints_alpha: float, _egreedy_epsilon: float, _DEBUG_SEGMENTS: int):
    global exp_id, algorithm, linucb_alpha, lints_alpha, egreedy_epsilon, DEBUG_SEGMENTS
    exp_id = _exp_id
    algorithm = _algorithm
    linucb_alpha = _linucb_alpha
    lints_alpha = _lints_alpha
    egreedy_epsilon = _egreedy_epsilon
    DEBUG_SEGMENTS = _DEBUG_SEGMENTS
