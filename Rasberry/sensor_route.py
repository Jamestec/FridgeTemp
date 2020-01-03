from data import get_latest_log, line_to_dic
from helpers import get_datetime_utc, get_datetime_here
from helpers import date_str, date_folder_str, time_str, datetime_str
from helpers import minus_time, parent, wake_reason

BUFFER = 12 * 24 # Whole day's worth
buffer = []

last_log = get_latest_log()["datetime"]

def sensor_record(data):
    if "wake" not in data and "humid" not in data:
        string = "{} Unknown: {}\n".format(datetime_str(), data)
        print(string)
        add_buffer(string)
        return ''

    log_datetime = get_datetime_utc()
    data = data.split("\n")
    to_print = []
    to_write = []
    for line in data:
        if "temp" in line:
            dic = line_to_dic(line)
            # Prep to print
            print_str = datetime_str(get_datetime_here(log_datetime))
            for key in dic:
                if key != "original" and key != "unknown":
                    print_str += " {}={}".format(key, dic[key])
            for unknown in dic["unknown"]:
                print_str += " {}".format(unknown)
            to_print.insert(0, print_str)
            global last_log
            last_log_temp = last_log
            if log_datetime >= last_log:
                # Make folder
                parent_folder = "sensor/" + parent(date_folder_str(log_datetime))
                if not os.path.exists(parent_folder):
                    os.makedirs(parent_folder)
                # Prep to write
                if dic["humid"] > 0:
                    path = "sensor/{}.txt".format(date_folder_str(log_datetime))
                    content = "{} {}\n".format(time_str(log_datetime), line)
                    to_write.insert(0, (path, content))
                last_log_temp = log_datetime
            else:
                to_print[0] += " <-- not logged"
            log_datetime = minus_time(log_datetime)
    for line in to_print:
        print(line)
        add_buffer(line)
    for path, content in to_write:
        with open(path, "a+") as FILE:
            FILE.write(content)
    last_log = last_log_temp
    print("")
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
