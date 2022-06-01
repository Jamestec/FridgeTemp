function submitFromTo() {
	let url = new URL(window.location.href);
	const dt = document.getElementsByClassName("dt-pick");
	for (let i = 0; i < dt.length; i++) {
		if (elementChanged.includes(dt[i].id) && dt[i].value != "") {
			url.searchParams.set(dt[i].name, dt[i].value);
		} else {
			url.searchParams.delete(dt[i].name);
		}
	}
	const loop = ["temp", "humid", "volt"];
	for (dataSeries of loop) {
		if (elementChanged.includes(dataSeries)) {
			url.searchParams.set(dataSeries + "_visible", visible[dataSeries]);
		} else {
			url.searchParams.delete(dataSeries + "_visible");
		}
	}
	const sensorCheck = document.getElementsByClassName("sensorCheckbox");
	for (let i = 0; i < sensorCheck.length; i++) {
		if (elementChanged.includes(sensorCheck[i].id) && sensorCheck[i].checked == false) {
			url.searchParams.set(sensorCheck[i].id + "_checked", sensorCheck[i].checked);
		} else {
			url.searchParams.delete(sensorCheck[i].id + "_checked");
		}
	}
	window.location.href = url.toString();
}

function clearURLParams() {
	let url = window.location.href;
	window.location.href = url.substring(0, url.indexOf("?"));
}

function resetdtpick() {
	let dt = new Date();
	let time = [("0" + dt.getHours()).slice(-2), ("0" + dt.getMinutes()).slice(-2)].join(":");
	document.getElementById("fromDate").valueAsDate  = dt;
	document.getElementById("fromDate").stepDown(1);
	document.getElementById("fromTime").defaultValue = time;
	document.getElementById("toDate").valueAsDate  = dt;
	document.getElementById("toTime").defaultValue = time;
}

function onChanged(id) {
	if (!elementChanged.includes(id)) {
		elementChanged.push(id);
	}
}

function prevDay() {
	document.getElementById("fromDate").stepDown(1);
	onChanged("fromDate");
	document.getElementById("toDate").stepDown(1);
	onChanged("toDate");
}

function nextDay() {
	document.getElementById("fromDate").stepUp(1);
	onChanged("fromDate");
	document.getElementById("toDate").stepUp(1);
	onChanged("toDate");
}

function showTextVersion() {
	// Add text output (TODO: beta)
	document.getElementById("textDiv").innerText = ""
	document.getElementById("btnText").innerText = "Loading...";
	document.getElementById("btnText").disabled = true;
	for (id in data) {
		let limit = data[id]["limit"];
		let times = data[id]["datetime"];
		let temp = data[id]["temp"];
		for (let i = 0; i < limit; i += 1) {
			let date = new Date(times[i]);
			let textData = []	
			textData.push({date:date, temp:temp[i]});
			textData.sort(function(a,b){
				return new Date(a.date) - new Date(b.date);
			});
			for (x in textData) {
				document.getElementById("textDiv").innerText += textData[x].date + ": " + textData[x].temp + "°C\n";
			}
		}
	}
	document.getElementById("btnText").innerText = "Finished";
}