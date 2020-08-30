import math
import time


def get_epoch_time():
    return int(round(time.time())) * 1000


def epoch_to_localtime(timestamp):
    timestamp /= 1000
    return time.strftime('%Y-%m-%d %H:%M:%S', time.localtime(timestamp))