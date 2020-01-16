DATA_FOLDER = 'FridgeTemp/Raspberry/sensor'
TIME_BETWEEN_READS = 180 # Seconds
BUFFER = 20 * 24 # Whole day's worth
VISIBLE_KEYS = ("temp_visible", "humid_visible", "volt_visible")

from helpers import get_local_time_diff, get_datetime_utc, get_datetime_here, \
                    date_str, date_folder_str, time_str, datetime_str, \
                    get_date, get_time, get_datetime, \
                    minus_time, add_day, parent, wake_reason, get_bool

from data import get_dirs, get_path_date, get_latest_log, get_latest_log_file, \
                 parse_val, line_to_dic, get_data, dic_to_graph, dic_to_graph_dic

from graph_route import graph_html, get_graph_defaults

from sensor_route import sensor_record, add_buffer, buffer_str
