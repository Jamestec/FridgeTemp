import os
from flask import Flask, request
from subprocess import check_output
from imports import graph_html, \
                    sensor_record, buffer_str, \
                    sensor_packet_record, sensor_packet_fin_record, \
                    sensor_packet_reset_record, \
                    sd_status_record, \
                    get_data, dic_to_graph, \
                    DATA_FOLDER

import logging
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

app = Flask(__name__)
DEBUG = True

@app.route('/sensor', methods=["POST", "PUT", "GET"])
def sensor():
    data = request.data.decode("utf-8")
    return sensor_record(data)

@app.route('/sensor_packet', methods=["POST"])
def sensor_packet():
    data = request.data.decode("utf-8")
    return sensor_packet_record(data)

@app.route('/sensor_packet_fin', methods=["PUT", "GET"])
def sensor_packet_fin():
    print("packet_fin")
    return sensor_packet_fin_record()

@app.route('/sensor_packet_reset', methods=["PUT", "GET"])
def sensor_packet_reset():
    print("packet_reset")
    return sensor_packet_reset_record()

@app.route('/sd_status', methods=["PUT", "GET"])
def sd_status():
    data = request.data.decode("utf-8")
    return sd_status_record(data)

@app.route('/', methods=["POST", "PUT", "GET"])
@app.route('/graph', methods=["POST", "PUT", "GET"])
def graph():
    return graph_html(request.values, "Fridge Stats")

@app.route('/dumpbuffer', methods=["POST", "PUT", "GET"])
def dumpbuffer():
    return buffer_str()

if __name__ == "__main__":
    ip_address = check_output(["hostname", "--all-ip-addresses"]).decode("UTF-8").split(" ")[0]
    print("My ip is: {}".format(ip_address))
    app.run(host=ip_address, port=8090, debug=DEBUG)
