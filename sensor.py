from datetime import datetime
from flask import Flask, request
import os
from time import time

import logging
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

app = Flask(__name__)
DEBUG = False

@app.route('/sensor', methods=["POST", "PUT", "GET"])
def sensor():
    splitted = request.data.decode("utf-8").split(" ")
    data = [0, 0, 0]
    for i in range(len(splitted)):
        data[i] = splitted[i]

    parent_folder = "sensor/" + parent(date_folder_str())
    if not os.path.exists(parent_folder):
        os.makedirs(parent_folder)

    print("{} {} {} {}\n".format(datetime_str(), data[0], data[1], wake_reason(data[2])))
    if get_float(data[1]) > 0:
        file = open("sensor/{}.txt".format(date_folder_str()), "a+")
        file.write("{} {} {} {}\n".format(time_str(), data[0], data[1], data[2]))
        file.close()
    return ''

def date_str():
    '''E.g. 2019-10-29'''
    return datetime.now().strftime("%Y-%m-%d")

def date_folder_str():
    '''E.g. 2019/10/29'''
    return datetime.now().strftime("%Y/%m/%d")

def parent(path):
    return path.rpartition("/")[0]

def time_str():
    '''E.g. 21:31:22'''
    return datetime.now().strftime("%H:%M:%S")

def datetime_str():
    '''E.g. 2019-10-29 21:31:22'''
    return "{} {}".format(date_str(), time_str())

def epoch():
    '''Time in seconds from 1970'''
    return time()

def datetime_from(epoch):
    '''Get datetime object from epoch'''
    return datetime.fromtimestamp(epoch)

def get_float(string):
    return float(string.rpartition(" ")[2][:-1])

def wake_reason(wake_int):
    wake_int = int(wake_int)
    if wake_int == 1:
        return "timer"
    if wake_int == 2:
        return "vibration"
    return "unknown"

if __name__ == "__main__":
    if not os.path.exists("sensor/"):
        os.mkdir("sensor/")
    app.run(host='192.168.1.122', port=8090, debug=DEBUG)
