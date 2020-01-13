from flask import render_template
from imports import get_datetime_utc, date_str, time_str, get_graph_defaults, \
                    get_data, dic_to_graph

def graph_html(request_values, title="untitled", html="sensor_graph_jquery.html"):
    from_dt, to_dt = get_graph_defaults(request_values)
    times, temp, humid, volt = dic_to_graph(get_data(
                                                    get_datetime_utc(from_dt),
                                                    get_datetime_utc(to_dt)))
    fromdate = date_str(from_dt)
    fromtime = time_str(from_dt)[:-3]
    todate = date_str(to_dt)
    totime = time_str(to_dt)[:-3]
    return render_template('sensor_graph_jquery.html', title=title,
                           times=times, temp=temp, humid=humid, volt=volt,
                           fromdate=fromdate, fromtime=fromtime, todate=todate, totime=totime)
