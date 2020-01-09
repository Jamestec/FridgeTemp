import os
from flask import Flask, request
from subprocess import check_output
from graph_route import graph_html
from sensor_route import sensor_record, buffer_str
from data import DATA_FOLDER, get_data, dic_to_graph

import logging
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

app = Flask(__name__)
DEBUG = True

@app.route('/sensor', methods=["POST", "PUT", "GET"])
def sensor():
    data = request.data.decode("utf-8")
    return sensor_record(data)

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
