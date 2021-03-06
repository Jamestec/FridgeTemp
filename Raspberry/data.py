import os
from datetime import datetime, timezone
from imports import get_datetime_utc, get_datetime_here, get_datetime_from_timestamp, \
                    date_folder_str, datetime_str, get_time, get_datetime, \
                    minus_time, add_day, wake_reason, \
                    DATA_FOLDER

def get_dirs(parent):
    '''Returns list of full relative path of sub-directories of parent'''
    return [os.path.join(parent, dir)
            for dir in os.listdir(parent)
            if os.path.isdir(os.path.join(parent, dir))]

def get_path_date(log):
    date = log.split('/')
    date = date[3:]
    date[-1] = date[-1].split('.')[0]
    return tuple(map(int, date))

def get_latest_log(log=None):
    '''log should be DATA_FOLDER/year/month/day.txt'''
    if log is None:
        log = get_latest_log_file()
    if log is None: # There are no records
        # Fake last log was 1 day ago
        return {"datetime": minus_time(get_datetime_utc(), 86400)}
    with open(log, 'r') as FILE:
        # Need to change get_latest_log_file to return file that is not empty
        # Need to change this function to return latest non-empty line
        last = FILE.read().splitlines()[-2] # Patch fix change -1 to -2
    date = get_path_date(log)
    return line_to_dic(last, date)

def get_latest_log_file():
    '''Get relative path to latest log file'''
    years = get_dirs(DATA_FOLDER)
    years.sort(reverse=True)
    for year in years:
        months = get_dirs(year)
        months.sort(reverse=True)
        for month in months:
            days = [os.path.join(month, day)
                    for day in os.listdir(month)
                    if not os.path.isdir(os.path.join(month, day))]
            days.sort(reverse=True)
            if len(days) > 0:
                return days[0]

def parse_val(key, val):
    f = ['temp', 'humid', 'volt', 'batVolt']
    try:
        if key in f:
            return float(val)
        if key == 'wake':
            return wake_reason(val)
        if key == 'time':
            return get_datetime_utc(get_datetime_from_timestamp(val))
    except:
        pass
    return val

def line_to_dic(line, date=None):
    '''
    Saves non key=val pairs in key 'unknown', original line in 'original'
    date is a tuple - year, month, day
    '''
    splitted = line.split(' ')
    dic = {}
    unknown = []
    for split in splitted:
        pair = split.split('=');
        if len(pair) < 2:
            time = get_time(pair[0])
            if time is not None and date is not None:
                dic['datetime'] = get_datetime(date, time)
            else:
                unknown.append(pair[0])
        else:
            dic[pair[0]] = parse_val(pair[0], pair[1])
    dic['unknown'] = unknown
    dic['original'] = line
    return dic

def get_data(start_dt=None, end_dt=None):
    end_dt = get_datetime_utc(end_dt)
    if start_dt is None:
        start_dt = minus_time(end_dt, 86400) # 1 day
    else:
        start_dt = get_datetime_utc(start_dt)
    dt = start_dt.replace(hour=0, minute=0, second=0)
    data = []
    while dt <= end_dt:
        path = os.path.join(DATA_FOLDER, date_folder_str(dt) + '.txt')
        try:
            with open(path, 'r') as FILE:
                date = get_path_date(path)
                for line in FILE.read().splitlines():
                    read = line_to_dic(line, date)
                    if read['datetime'] >= start_dt and read['datetime'] <= end_dt:
                        data.append(read)
        except:
            pass
        dt = add_day(dt)
    return data

def dic_to_graph(data_dic):
    data = {}
    sensor_ids = []
    for line in data_dic:
        sensor_id = "0"
        if "id" in line:
            sensor_id = line["id"]
        if sensor_id not in sensor_ids:
            sensor_ids.append(sensor_id)
        if sensor_id not in data:
            data[sensor_id] = {"datetime": [], "temp": [], "humid": [], "volt": []}
        data[sensor_id]["datetime"].append(datetime_str(get_datetime_here(line['datetime'])))
        data[sensor_id]["temp"].append(line['temp'])
        if 'humid' in line:
            data[sensor_id]["humid"].append(line['humid'])
        else:
            data[sensor_id]["humid"].append(0)
        if 'volt' in line:
            data[sensor_id]["volt"].append(line['volt'])
        else:
            data[sensor_id]["volt"].append(0)
    for sensor in data:
        data[sensor]["limit"] = len(data[sensor]["temp"])
    sensor_ids = sorted(sensor_ids)
    return data, sensor_ids

def dic_to_graph_dic(data_dic):
    temp = []
    humid = []
    volt = []
    for data in data_dic:
        dt_str = data['datetime'].timestamp()
        temp.append({'x': dt_str, 'y': data['temp']})
        humid.append({'x': dt_str, 'y': data['humid']})
        volt.append({'x': dt_str, 'y': data['volt']})
    return temp, humid, volt
