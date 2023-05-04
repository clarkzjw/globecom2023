import argparse
from multiprocessing import Process

from config import init
from downloader import Downloader
from player import MockPlayer

if __name__ == "__main__":
    global exp_id, algorithm, linucb_alpha, lints_alpha, egreedy_epsilon, DEBUG_SEGMENTS

    parser = argparse.ArgumentParser(description="Adaptive video streaming with contextual bandits and MPQUIC")
    parser.add_argument("--exp_id", default="default", help="experiment id")
    parser.add_argument("--scheduler", choices=["roundrobin", "minrtt", "contextual_bandit"], default="roundrobin", help="scheduler")
    parser.add_argument("--algorithm", choices=["LinUCB", "LinTS", "LinGreedy"], default="LinUCB", help="scheduling algorithm")
    parser.add_argument("--linucb_alpha", type=float, default=0.1, help="alpha for LinUCB")
    parser.add_argument("--lints_alpha", type=float, default=0.1, help="alpha for LinTS")
    parser.add_argument("--egreedy_epsilon", type=float, default=0.1, help="epsilon for epsilon-greedy")
    parser.add_argument("--nb_segment", type=int, default=100, help="number of segments to download")

    args = parser.parse_args()

    init(args.exp_id, args.algorithm, args.linucb_alpha, args.lints_alpha, args.egreedy_epsilon, args.nb_segment)

    mplayer = MockPlayer(exp_id=args.exp_id)

    mplayer_process = Process(target=mplayer.play)
    mplayer_process.start()

    mdownloader = Downloader(scheduler=args.scheduler, algorithm=args.algorithm)
    mdownload_process = Process(target=mdownloader.main_loop)
    mdownload_process.start()

    mplayer_process.join()
    mdownload_process.join()
