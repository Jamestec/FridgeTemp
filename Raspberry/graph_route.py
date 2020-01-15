from flask import render_template
from imports import get_datetime_utc, get_datetime_here, date_str, time_str, \
                    get_date, get_time, minus_time, \
                    get_data, dic_to_graph, get_bool, \
                    VISIBLE_KEYS

def graph_html(request_values, title="untitled", html="sensor_graph_jquery.html"):
    from_dt, to_dt, vis, elementChanged = get_graph_defaults(request_values)
    times, temp, humid, volt = dic_to_graph(get_data(
                                                    get_datetime_utc(from_dt),
                                                    get_datetime_utc(to_dt)))
    fromdate = date_str(from_dt)
    fromtime = time_str(from_dt)[:-3]
    todate = date_str(to_dt)
    totime = time_str(to_dt)[:-3]
    return render_template('sensor_graph_jquery.html', title=title,
                           times=times, temp=temp, humid=humid, volt=volt,
                           fromdate=fromdate, fromtime=fromtime, todate=todate, totime=totime,
                           temp_visible=vis[0], humid_visible=vis[1], volt_visible=vis[2],
                           elementChanged=elementChanged)

def get_graph_defaults(request_values):
    elementChanged = []
    to_dt = get_datetime_here()
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

    return from_dt, to_dt, visible, elementChanged
