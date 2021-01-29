from flask import render_template
from imports import get_datetime_utc, get_datetime_here, date_str, time_str, \
                    get_date, get_time, minus_time, \
                    get_data, dic_to_graph, get_bool, \
                    sd_stat, TIME_BETWEEN_READS, VISIBLE_KEYS

def graph_html(request_values, title="untitled", html="sensor_graph_jquery.html"):
    from_dt, to_dt, vis, elementChanged, sensor_unchecked = get_graph_defaults(request_values)
    data, sensor_ids = dic_to_graph(get_data(   get_datetime_utc(from_dt),
                                    get_datetime_utc(to_dt)))
    for un in sensor_unchecked:
        data.pop(un[6:], None)
    fromdate = date_str(from_dt)
    fromtime = time_str(from_dt)[:-3]
    todate = date_str(to_dt)
    totime = time_str(to_dt)[:-3]
    global sd_stat
    return render_template(html, title=title, sd_status = str(sd_stat).lower(),
                           data=data, sensor_ids=sensor_ids, sensor_unchecked=sensor_unchecked,
                           fromdate=fromdate, fromtime=fromtime, todate=todate, totime=totime,
                           temp_visible=vis[0], humid_visible=vis[1], volt_visible=vis[2],
                           elementChanged=elementChanged, TIME_BETWEEN_READS=TIME_BETWEEN_READS)

def get_graph_defaults(request_values):
    elementChanged = []
    to_dt = get_datetime_here()
    # Date stuff
    if request_values.has_key("toDate"):
        result = get_date(request_values["toDate"])
        if result is not None:
            to_dt = to_dt.replace(year=result[0], month=result[1], day=result[2])
            elementChanged.append("toDate")
    if request_values.has_key("toTime"):
        result = get_time(request_values["toTime"])
        if result is not None:
            to_dt = to_dt.replace(hour=result[0], minute=result[1], second=result[2])
            elementChanged.append("toTime")

    from_dt = minus_time(to_dt, 86400) # 1 day
    if request_values.has_key("fromDate"):
        result = get_date(request_values["fromDate"])
        if result is not None:
            from_dt = from_dt.replace(year=result[0], month=result[1], day=result[2])
            elementChanged.append("fromDate")
    if request_values.has_key("fromTime"):
        result = get_time(request_values["fromTime"])
        if result is not None:
            from_dt = from_dt.replace(hour=result[0], minute=result[1], second=result[2])
            elementChanged.append("fromTime")
    # Line visibility
    visible = []
    for key in VISIBLE_KEYS:
        # Set defaults
        item = "false"
        if key == "temp_visible":
            item = "true";
        # Override with URL params if available
        if request_values.has_key(key):
            result = request_values[key]
            if result is not None and result != item:
                item = result
                elementChanged.append(key.split("_")[0])
        visible.append(item)

    # Checkbox (sensor) unchecked
    sensor_unchecked = []
    [x.split("_")[0] for x in request_values.keys() if "checked" in x and not get_bool(request_values[x])]
    for key in request_values.keys():
        if "checked" in key and not get_bool(request_values[key]):
            x = key.split("_")[0]
            sensor_unchecked.append(x)
            elementChanged.append(x)
    return from_dt, to_dt, visible, elementChanged, sensor_unchecked
