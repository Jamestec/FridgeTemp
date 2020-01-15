function submitFromTo() {
	let url = new URL(window.location.href);
	const params = document.getElementsByClassName("dt-pick");
	for (i = 0; i < params.length; i++) {
		if (elementChanged.includes(params[i].id) && params[i].value != "") {
			url.searchParams.set(params[i].name, params[i].value);
		} else {
			url.searchParams.delete(params[i].name);
		}
	}
	let loop = ["temp", "humid", "volt"];
	for (dataSeries of loop) {
		if (elementChanged.includes(dataSeries)) {
			url.searchParams.set(dataSeries + "_visible", visible[dataSeries]);
		} else {
			url.searchParams.delete(dataSeries + "_visible");
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
