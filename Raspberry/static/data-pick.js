function submitFromTo() {
	let url = new URL(window.location.href);
	const params = document.getElementsByClassName("dt-pick");
	for (i = 0; i < params.length; i++) {
		if (params[i].value != "") {
			url.searchParams.set(params[i].name, params[i].value);
		} else {
			url.searchParams.delete(params[i].name);
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
