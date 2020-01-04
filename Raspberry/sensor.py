import os
from flask import Flask, request, render_template
from subprocess import check_output
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
    times, temp, humid, volt = dic_to_graph(get_data())
    return render_template('sensor_graph.html', title='Graph of previous 24 hours',
                           times=times, temp=temp, humid=humid, volt=volt)

@app.route('/dumpbuffer', methods=["POST", "PUT", "GET"])
def dumpbuffer():
    return buffer_str()

if __name__ == "__main__":
    if not os.path.exists(DATA_FOLDER):
        os.mkdir(DATA_FOLDER)
    ip_address = check_output(["hostname", "--all-ip-addresses"]).decode("UTF-8")
    print("My ip is: {}".format(ip_address))
    app.run(host=ip_address, port=8090, debug=DEBUG)
