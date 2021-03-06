from datetime import datetime, timedelta, timezone
from imports import TIME_BETWEEN_READS

''' Add tzinfo=timezone.utc '''
def get_datetime_utc(dt=None):
    if dt is None:
        dt = datetime.now(timezone.utc)
    if dt.tzinfo is None:
        dt = datetime.fromtimestamp(dt.timestamp(), timezone.utc)
    return dt

''' Strip tzinfo=timezone.utc '''
def get_datetime_here(dt=None):
    if dt is None:
        return datetime.now()
    return datetime.fromtimestamp(dt.timestamp())

def get_datetime_from_timestamp(timestamp=None):
    if timestamp == None:
        timestamp = 0
    return datetime.fromtimestamp(int(timestamp))

def date_str(date=None):
    '''E.g. 2019-10-29'''
    if date is None:
        date = get_datetime_utc()
    return date.strftime("%Y-%m-%d")

def date_folder_str(date=None):
    '''E.g. 2019/10/29'''
    if date is None:
        date = get_datetime_utc()
    return date.strftime("%Y/%m/%d")

def time_str(time=None):
    '''E.g. 21:31:22'''
    if time is None:
        time = get_datetime_utc()
    return time.strftime("%H:%M:%S")

def datetime_str(dt=None):
    '''E.g. 2019-10-29 21:31:22'''
    return "{} {}".format(date_str(dt), time_str(dt))

def get_date(string, sep="-"):
    splitted = string.split(sep)
    if len(splitted) > 2:
        return tuple(map(int, splitted))
    return None

def get_time(string):
    splitted = string.split(":")
    if len(splitted) == 2:
        splitted.append("0")
    if len(splitted) > 2:
        return tuple(map(int, splitted))
    return None

def get_datetime(date, time):
    '''date and time are tuples - year, month, day, hour, minute, seconds'''
    return datetime(date[0], date[1], date[2], time[0], time[1], time[2], tzinfo=timezone.utc)

def minus_time(dt, seconds=TIME_BETWEEN_READS):
    return dt - timedelta(seconds=seconds)

def add_day(dt):
    return dt + timedelta(days=1)

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

def get_bool(string):
    string = string.lower()
    true = ['true']
    false = ['false']
    if string in true:
        return True
    if string in false:
        return False
    return None
