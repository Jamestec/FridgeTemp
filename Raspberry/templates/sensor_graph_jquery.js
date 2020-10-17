window.onload = function () {

const limit = {{times | length}};
const times = {{times | safe}};
const temp = {{temp}};
const humid = {{humid}};
const volt = {{volt}};

const temp_color = "#C0504E";
const humid_color = "#4F81BC";
const volt_color = "#f0a502";

// Determine minimum (and max) voltage for axis label
let volt_min = Math.min(...volt) - 0.01;
if (volt_min > 3.75 || volt_min == 0 - 0.01) {
	volt_min = 3.75;
}
const volt_max = 4.25

elementChanged = {{elementChanged}};
visible = {temp: {{temp_visible}}, humid: {{humid_visible}}, volt: {{volt_visible}}};

// Setup graph line data stuff
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
// Prep for parsing server data to line data stuff
let prevDate = new Date(document.getElementById("fromDate").value + " "+
			document.getElementById("fromTime").value);
var TIME_BETWEEN_READS = {{TIME_BETWEEN_READS}} * 1000;
// Determine if {{TIME_BETWEEN_READS}} is appropriate for most of data
// This may be because ESP32 was changed, but not Raspberry Pi
if (limit > 1) {
	let sample = Math.abs(new Date(times[1]) - new Date(times[0]));
	sample += Math.abs(new Date(times[limit / 2]) - new Date(times[(limit / 2) - 1]));
	sample += Math.abs(new Date(times[limit - 1]) - new Date(times[limit - 2]));
	sample /= 3;
	if (sample > TIME_BETWEEN_READS) {
		TIME_BETWEEN_READS = sample;
	}
}
min_temp = 500;
max_temp = -500;
const THRESHOLD = TIME_BETWEEN_READS / 3;
let sections = {};
// Loop to fill data points (line data stuff) + other
for (let i = 0; i < limit; i += 1) {
	let date = new Date(times[i]);
	if (date - prevDate > TIME_BETWEEN_READS + THRESHOLD) {
		// Missing some reads, fill it with fluf so no line
		prevDate = new Date(prevDate.getTime() + TIME_BETWEEN_READS);
		addDataPoint(prevDate, tempData, humidData, voltData,
				undefined, undefined, undefined);
	}
	if (isGoodTempRead(temp[i])) {
		// Determine min and max
		if (temp[i] > max_temp) { max_temp = temp[i]; }
		if (temp[i] < min_temp) { min_temp = temp[i]; }
		// Do min/max for sections
		doSections(sections, date, temp[i], TIME_BETWEEN_READS);
	}
	// Add data point
	addDataPoint(date, tempData, humidData, voltData,
			temp[i], humid[i], volt[i]);
	prevDate = date;
}
// There were no good sensor reads :(, no min/max
if (min_temp == 500) min_temp = undefined;
if (max_temp == -500) max_temp = undefined;
// Fluff the last sensor to now so there's space on graph
let endDate = new Date(document.getElementById("toDate").value + " "+
			document.getElementById("toTime").value);
if (endDate - prevDate > TIME_BETWEEN_READS + THRESHOLD) {
	addDataPoint(endDate, tempData, humidData, voltData,
				undefined, undefined, undefined);
}
// Add line data stuff to graph
data.push(tempData);
data.push(humidData);
data.push(voltData);

// Graph statistics
let stats = "<b>Total date range stats:</b><br>Temperature min/max: " + min_temp + "°C/" + max_temp + "°C" + "<br><br>"
	+ "<b>Sections:</b><br>" + doSectionString(sections);
document.getElementById("stats").innerHTML = stats;

// Set graph options/assign data
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

// Battery low voltage warning
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

function addDataPoint(date, tempData, humidData, voltData, temp, humid, volt) {
	if (!isGoodTempRead(temp)) {
		temp = undefined;
	}
	if (!isGoodHumidRead(humid)) {
		humid = undefined;
	}
	tempData.dataPoints.push({
		x: date,
		y: temp,
		markerSize: 7
	});
	humidData.dataPoints.push({
		x: date,
		y: humid,
		markerSize: 7
	});
	if (volt < 1) volt = undefined;
	voltData.dataPoints.push({
		x: date,
		y: volt,
		markerSize: 7
	});
}

function isGoodTempRead(temp) {
	return temp > -40; // Specifically, bad temp read is -46.58
}

function isGoodHumidRead(humid) {
	return humid >= 0;
}

// Is the time close to opening or closing time
function isStartOfSection(date) {
	let startHour = 9;
	let endHour = 19; // 7pm
	if (date.getDay() == 0 || date.getDay() == 6) { // Sun or Sat
		endHour = 18; // 6pm
	}
	let minutesBetween = {{TIME_BETWEEN_READS}} / 60;
	return (date.getMinutes() <= minutesBetween &&
			(date.getHours() == startHour || date.getHours() == endHour));
}

function getEndHour(dayOfWeek) {
	if (dayOfWeek == 0 || dayOfWeek == 6) { // Sun or Sat
		return 18; // 6pm
	}
	return 19; // 7pm
}

// Get end of section datetime as string
function getSectionTime(date) {
	let startHour = 9;
	let endHour = getEndHour(date.getDay());
	// Between midnight and open
	let hour = " Morn";
	// Between open and close
	if (date.getHours() >= startHour && date.getHours() < endHour) {
		hour = " Night";
	}
	let addADay = 0;
	// Between close and midnight
	if (date.getHours() >= endHour) {
		addADay = 1;
	}
	let ret = new Date(date);
	ret.setDate(date.getDate() + addADay);
	//ret.setHours(hour);
	let day = ret.getDate();
	if (day < 10) {
		day = "0" + day;
	}
	let month = ret.getMonth() + 1;
	if (month < 10) {
		month = "0" + month;
	}
	return ret.getFullYear() + "/" + month + "/" + day + hour;
}

function doSections(sections, date, temp, TIME_BETWEEN_READS) {
	if (isStartOfSection(date)) {
		let endSec = getSectionTime(date);
		if (endSec in sections) {
			//sections[endSec].push(date + " " + temp + "<br>");
			if (temp < sections[endSec]["min"]) {
				sections[endSec]["min"] = temp;
			} else if (temp > sections[endSec]["max"]) {
				sections[endSec]["max"] = temp;
			}
		} else {
			//sections[endSec] = [date + " " + temp + "<br>"];
			sections[endSec] = {
				"min": temp,
				"max": temp
			};
		}
	} else {
		let endSec = getSectionTime(date);
		if (endSec in sections) {
			//sections[endSec].push(date + " " + temp + "<br>");
			if (temp < sections[endSec]["min"]) {
				sections[endSec]["min"] = temp;
			} else if (temp > sections[endSec]["max"]) {
				sections[endSec]["max"] = temp;
			}
			if (parseEndSectionString(endSec) - date <= TIME_BETWEEN_READS) {
				sections[endSec]["end"] = true;
			}
		}
	}
}

function parseEndSectionString(str) {
	let s = str.split(" ");
	let d = new Date(s[0]);
	if (s[1] == "Morn") {
		d.setHours(9);
	} else {
		d.setHours(getEndHour(d.getDay()));
	}
	return d;
}

function doSectionString(sections) {
	let sectionString = "";
	let prevSub = "";
	for (key in sections) {
		if (!("end" in sections[key])) {
			sectionString += "<p style=\"color:red\">";
		}
		let sub = key.split(" ");
		if (prevSub == sub[0]) {
			sectionString += "---/---/-------- " + sub[1];
		} else {
			sectionString += key;
		}
		prevSub = sub[0];
		sectionString += ": " + sections[key]["min"].toFixed(2) + "°C/" + sections[key]["max"].toFixed(2) + "°C";
		if (!("end" in sections[key])) {
			sectionString += " --- incomplete data</p>";
		}
		sectionString += "<br>";
	}
	return sectionString;
}

}
