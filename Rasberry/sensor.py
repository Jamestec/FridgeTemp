from datetime import datetime, timedelta
from flask import Flask, request
import os
from time import time
from helpers import date_str, date_folder_str, time_str, datetime_str, parent, wake_reason

import logging
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

app = Flask(__name__)
DEBUG = True

TIME_BETWEEN_READS = 10 # Seconds

@app.route('/sensor', methods=["POST", "PUT", "GET"])
def sensor():
    data = request.data.decode("utf-8")

    if "wake" not in data and "humid" not in data:
        print("{} Unknown: {}\n".format(datetime_str(), data))
        return ''

    log_datetime = datetime.now()
    data = data.split("\n")
    to_print = []
    to_write = []
    for line in data:
        if "temp" in line:
            dic = lineToDic(line)
            # Printing
            print_str = datetime_str(log_datetime)
            for key in dic:
                if key != "original" and key != "unknown":
                    print_str += " {}={}".format(key, dic[key])
            for unknown in dic["unknown"]:
                print_str += " {}".format(unknown)
            to_print.insert(0, print_str)
            # Make folder
            parent_folder = "sensor/" + parent(date_folder_str(log_datetime))
            if not os.path.exists(parent_folder):
                os.makedirs(parent_folder)
            # Write to file
            if dic["humid"] > 0:
                path = "sensor/{}.txt".format(date_folder_str(log_datetime))
                content = "{} {}\n".format(time_str(log_datetime), line)
                to_write.insert(0, (path, content))
            log_datetime = minus_time(log_datetime)
    for line in to_print:
        print(line)
    for path, content in to_write:
        with open(path, "a+") as FILE:
            FILE.write(content)
    print("")
    return ''

def parse_val(key, val):
    f = ["temp", "humid", "volt", "batVolt"]
    try:
        if key in f:
            return float(val)
        if key == "wake":
            return wake_reason(val)
    except:
        pass
    return val

def lineToDic(line):
    '''Saves non key=val pairs in key "unknown", original line in "original"'''
    splitted = line.split(" ")
    dic = {}
    unknown = []
    for split in splitted:
        pair = split.split("=");
        if len(pair) < 2:
            unknown.append(pair[0])
        else:
            dic[pair[0]] = parse_val(pair[0], pair[1])
    dic["unknown"] = unknown
    dic["original"] = line
    return dic

def minus_time(dt):
    return dt - timedelta(seconds=TIME_BETWEEN_READS)

if __name__ == "__main__":
    if not os.path.exists("sensor/"):
        os.mkdir("sensor/")
    app.run(host='192.168.1.122', port=8090, debug=DEBUG)
