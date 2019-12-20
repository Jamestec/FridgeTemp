from datetime import datetime

def date_str(date=None):
    '''E.g. 2019-10-29'''
    if date is None:
        date = datetime.now()
    return date.strftime("%Y-%m-%d")

def date_folder_str(date=None):
    '''E.g. 2019/10/29'''
    if date is None:
        date = datetime.now()
    return date.strftime("%Y/%m/%d")

def time_str(time=None):
    '''E.g. 21:31:22'''
    if time is None:
        time = datetime.now()
    return time.strftime("%H:%M:%S")

def datetime_str(dt=None):
    '''E.g. 2019-10-29 21:31:22'''
    return "{} {}".format(date_str(dt), time_str(dt))

def parent(path):
    return path.rpartition("/")[0]

def wake_reason(wake_int):
    wake_int = int(wake_int)
    if wake_int == 0:
        return "unknown"
    if wake_int == 2:
        return "vibration"
    if wake_int == 4:
        return "timer"
    return str(wake_int)
