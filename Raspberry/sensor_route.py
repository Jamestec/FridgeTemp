import os
import sys
from imports import get_latest_log, line_to_dic, \
                    get_datetime_utc, get_datetime_here, get_datetime_from_timestamp, \
                    date_str, date_folder_str, time_str, datetime_str, \
                    minus_time, get_bool, parent, wake_reason, \
                    BUFFER, DATA_FOLDER, PACKET_TIMEOUT

buffer = []
sd_stat = True

if not os.path.exists(DATA_FOLDER):
    os.mkdir(DATA_FOLDER)

last_log = {}

def sensor_record(data):
    if "wake" not in data and "humid" not in data:
        string = "{} Unknown: {}\n".format(datetime_str(), data)
        print(string)
        add_buffer(string)
        return ''

    global last_log
    last_log_new = last_log.copy() # Herp derp
    log_datetime = get_datetime_utc()
    data = data.split("\n")
    to_print = []
    to_write = []
    not_logged = False
    last_time = None # This is for "fake" LolinD32 that can't keep track of time properly
    for i in range(len(data) - 1, -1, -1):
        line = data[i]
        if "temp" in line:
            dic = line_to_dic(line)
            sensor_id = "00"
            if "id" in dic:
                sensor_id = dic["id"]
            if "time" in dic and dic["time"] > get_datetime_utc(get_datetime_from_timestamp(1000000000)) and not dic["time"] == last_time:
                log_datetime = dic["time"]
                last_time = log_datetime
            # Prep to print
            print_str = datetime_str(get_datetime_here(log_datetime))
            for key in dic:
                if key != "original" and key != "unknown":
                    print_str += " {}={}".format(key, dic[key])
            for unknown in dic["unknown"]:
                print_str += " {}".format(unknown)
            to_print.insert(0, print_str)
            if sensor_id not in last_log or log_datetime >= last_log[sensor_id]:
                # Prep to write
                path = os.path.join(DATA_FOLDER, date_folder_str(log_datetime) + ".txt")
                content = "{} {}\n".format(time_str(log_datetime), line)
                to_write.insert(0, (path, content))
                if sensor_id not in last_log_new or log_datetime > last_log_new[sensor_id]:
                    last_log_new[sensor_id] = log_datetime
            else:
                to_print[0] += " <-- not logged (datetime past last log time)"
                not_logged = True
            if dic["temp"] < -40:
                to_print[0] += " <- bad sensor record"
            log_datetime = minus_time(log_datetime)
    for line in to_print:
        print(line)
        add_buffer(line)
    for path, content in to_write:
        # Make folder
        parent_folder = parent(path)
        if not os.path.exists(parent_folder):
            os.makedirs(parent_folder, mode=0o777, exist_ok=True)
        # Write
        with open(path, "a+") as FILE:
            FILE.write(content)
        try:
            os.chmod(path, 0o666) # Change permission to be read/write for everyone
        except OSError:
            pass
    last_log.clear() # This actually frees memory (not required but for my memory)
    last_log = last_log_new
    print("")
    sys.stdout.flush()
    if not_logged: # Dump for inspection later
        dt = get_datetime_utc()
        path = os.path.join(DATA_FOLDER, date_str(dt) + "_" + time_str(dt) + ".txt")
        # Make folder
        parent_folder = parent(path)
        if not os.path.exists(parent_folder):
            os.makedirs(parent_folder, mode=0o777, exist_ok=True)
        # Write
        with open(path, "a+") as FILE:
            FILE.write(to_print)
        try:
            os.chmod(path, 0o666)
        except OSError:
            pass
    return ''

def add_buffer(string):
    global buffer
    buffer.insert(0, string)
    if len(buffer) > BUFFER:
        del buffer[BUFFER - 1]

def buffer_str():
    string = ""
    for line in buffer:
        string += line + "<br/>"
    return string

packet = ""
packet_count = 0
packet_time = None
def sensor_packet_record(data):
    global packet, packet_count, packet_time
    if "wake" not in data and "temp" not in data:
        string = "{} Unknown: {}\n".format(datetime_str(), data)
        print(string)
        add_buffer(string)
        return ''
    # First packet
    if packet_count == 0:
        packet_time = get_datetime_utc()
    if ((get_datetime_utc() - packet_time).total_seconds() > PACKET_TIMEOUT):
        print(f"Packet timed out: {(get_datetime_utc() - packet_time).total_seconds()}\n")
        sensor_packet_fin_record()
    packet_count += 1
    packet += data
    packet_time = get_datetime_utc()
    print(f"packet {packet_count}:\n{data}")
    return str(packet_count)

def sensor_packet_fin_record():
    global packet
    if (len(packet) != 0):
        sensor_record(packet)
    sensor_packet_reset_record()
    return ''

def sensor_packet_reset_record():
    global packet, packet_count, packet_time
    packet = ""
    packet_count = 0
    packet_time = None
    return ''

def sd_status_record(data):
    global sd_stat
    print(f"sd_status: {data}")
    sd_stat = get_bool(data)
    return data
