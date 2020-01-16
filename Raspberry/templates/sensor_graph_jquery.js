window.onload = function () {

const limit = {{times | length}};
const times = {{times | safe}};
const temp = {{temp}};
const humid = {{humid}};
const volt = {{volt}};

const temp_color = "#C0504E";
const humid_color = "#4F81BC";
const volt_color = "#f0a502";

let volt_min = Math.min(...volt) - 0.01;
if (volt_min > 3.75 || volt_min == 0) {
	volt_min = 3.75;
}
const volt_max = 4.25

elementChanged = {{elementChanged}};
visible = {temp: {{temp_visible}}, humid: {{humid_visible}}, volt: {{volt_visible}}};

let data = [];
let tempData = {
	type: "line",
	name: "Temperature",
	id: "temp",
	color: temp_color,
	xValueType: "dateTime",
	axisYType: "primary",
        axisYIndex: 0,
	showInLegend: true,
	visible: visible["temp"],
	xValueFormatString: "DD/MM/YY HH:mm:ss",
	yValueFormatString: "##.#0°C",
};
let humidData = {
	type: "line",
	name: "Humidity",
	id: "humid",
	color: humid_color,
	xValueType: "dateTime",
	axisYType: "secondary",
        axisYIndex: 0,
	showInLegend: true,
	visible: visible["humid"],
	xValueFormatString: "DD/MM/YY HH:mm:ss",
	yValueFormatString: "##.#0 '%'",
};
let voltData = {
	type: "line",
	name: "Battery Voltage",
	id: "volt",
	color: volt_color,
	xValueType: "dateTime",
	axisYType: "secondary",
        axisYIndex: 1,
	showInLegend: true,
	visible: visible["volt"],
	xValueFormatString: "DD/MM/YY HH:mm:ss",
	yValueFormatString: "#.## v",
};
tempData.dataPoints = [];
humidData.dataPoints = [];
voltData.dataPoints = [];
for (let i = 0; i < limit; i += 1) {
	let date = new Date(times[i]);
	tempData.dataPoints.push({
		x: date,
		y: temp[i],
		markerSize: 7
	});
	humidData.dataPoints.push({
		x: date,
		y: humid[i],
		markerSize: 7
	});
	if (volt[i] < 1) volt[i] = undefined;
	voltData.dataPoints.push({
		x: date,
		y: volt[i],
		markerSize: 7
	});
}
data.push(tempData);
data.push(humidData);
data.push(voltData);

let options = {
	zoomEnabled: true,
	rangeChanged: function(e) {remURL();},
	exportEnabled: true,
	title: {
		text: "{{title}}",
		fontSize: 20,
		fontWeight: "no"
	},
	axisY: {
		lineColor: temp_color,
		labelFontColor: temp_color,
		tickColor: temp_color,
		includeZero: false,
		stripLines:[
			{value:2, color:"red", thickness:3},
			{value:8, color:"red", thickness:3},
		]
	},
	axisY2: [
	{
		lineColor: humid_color,
		labelFontColor: humid_color,
		tickColor: humid_color,
		includeZero: false
	}, {
		lineColor: volt_color,
		labelFontColor: volt_color,
		tickColor: volt_color,
		includeZero: false
	}],
	toolTip: {
		shared: true
	},
	legend: {
		cursor: "pointer",
		itemclick: toggleDataSeries,
		verticalAlign: "top"
	},
	data: data
};

if (visible.volt) {
	options.axisY2[1].minimum = volt_min;
	options.axisY2[1].maximum = volt_max;
}


$("#resizable").resizable({
	create: function (event, ui) {
		//Create chart.
		$("#chartContainer").CanvasJSChart(options);
		remURL();
	},
	resize: function (event, ui) {
		//Update chart size according to its container size.
		$("#chartContainer").CanvasJSChart().render();
		remURL();
	}
});


if (volt_min <= 3.77) {
	let count = 0;
	let good = volt.length * 0.8;
	for (item of volt) {
		if (item <= 3.77) {
			count += 1;
			if (count > good) {
				let loop = document.getElementsByClassName("voltLowWarn");
				for (item of loop) {
					item.style.visibility = "visible";
				}
				break;
			}
		}
	}
}

min_temp = Math.min(...temp);
max_temp = Math.max(...temp);
document.getElementById("stats").innerHTML = 
	"Max temp = " + max_temp + "°C<br>Min temp = " + min_temp + "°C";

document.getElementById("loadingGraph").remove();

function toggleDataSeries(e) {
	if (typeof (e.dataSeries.visible) === "undefined" || e.dataSeries.visible) {
		visible[e.dataSeries.id] = false;
		switch (e.dataSeries.name) {
			case "Temperature":
				e.chart.axisY[0].set("stripLines", []);
				onChanged("temp");
				break;
			case "Humidity":
				onChanged("humid");
				break;
			case "Battery Voltage":
				e.chart.axisY2[1].set("minimum", 0);
				e.chart.axisY2[1].set("maximum", 0);
				onChanged("volt");
				break;
		}
	} else {
		visible[e.dataSeries.id] = true;
		switch (e.dataSeries.name) {
			case "Temperature":
				e.chart.axisY[0].set("stripLines", [
					{value:2, color:"red", thickness:3},
					{value:8, color:"red", thickness:3},
				]);
				onChanged("temp");
				break;
			case "Humidity":
				onChanged("humid");
				break;
			case "Battery Voltage":
				e.chart.axisY2[1].set("minimum", volt_min);
				e.chart.axisY2[1].set("maximum", volt_max);
				onChanged("volt");
				break;
		}
	}
	e.dataSeries.visible = visible[e.dataSeries.id];
	e.chart.render();
	remURL();
}

function remURL() {
	// Because it gets in the way of resizing
	$("a").remove(".canvasjs-chart-credit");
}

}