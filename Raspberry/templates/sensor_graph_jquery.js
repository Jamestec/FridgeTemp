window.onload = function () {

const limit = {{times | length}};
const times = {{times | safe}};
const temp = {{temp}};
const humid = {{humid}};
const volt = {{volt}};

const temp_color = "#C0504E";
const humid_color = "#4F81BC";
const volt_color = "#f0a502";

let data = [];
let tempData = {
	type: "line",
	name: "Temperature",
	color: temp_color,
	xValueType: "dateTime",
	axisYType: "primary",
        axisYIndex: 0,
	showInLegend: true,
	xValueFormatString: "DD/MM/YY HH:mm:ss",
	yValueFormatString: "##.#0 C",
};
let humidData = {
	type: "line",
	name: "Humidity",
	color: humid_color,
	xValueType: "dateTime",
	axisYType: "secondary",
        axisYIndex: 0,
	showInLegend: true,
	xValueFormatString: "DD/MM/YY HH:mm:ss",
	yValueFormatString: "##.#0 '%'",
};
let voltData = {
	type: "line",
	name: "Battery Voltage",
	color: volt_color,
	xValueType: "dateTime",
	axisYType: "secondary",
        axisYIndex: 1,
	showInLegend: true,
	visible: false,
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

function remURL() {
	// Because it gets in the way of resizing
	$("a").remove(".canvasjs-chart-credit");
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

function toggleDataSeries(e) {
	if (typeof (e.dataSeries.visible) === "undefined" || e.dataSeries.visible) {
		e.dataSeries.visible = false;
		switch (e.dataSeries.name) {
			case "Temperature":
				e.chart.axisY[0].set("stripLines", []);				
				break;
			case "Battery Voltage":
				e.chart.axisY2[1].set("minimum", 0);
				e.chart.axisY2[1].set("maximum", 0);
				break;
		}
	} else {
		e.dataSeries.visible = true;
		switch (e.dataSeries.name) {
			case "Temperature":
				e.chart.axisY[0].set("stripLines", [
					{value:2, color:"red", thickness:3},
					{value:8, color:"red", thickness:3},
				]);			
				break;
			case "Battery Voltage":
				e.chart.axisY2[1].set("minimum", 3.5);
				e.chart.axisY2[1].set("maximum", 4.3);
				break;
		}
	}
	e.chart.render();
	remURL();
}

}