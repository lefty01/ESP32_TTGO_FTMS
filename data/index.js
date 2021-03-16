

function buttonclick(e) {
    console.log(e.id);

    if (e.id === "speed_up") {
	fetch("/speed/up", {
	    method: 'PUT'
	});
    }
    if (e.id === "speed_down") {
	fetch("/speed/down", {
	    method: 'PUT'
	});
    }
    if (e.id === "incline_up") {
	fetch("/incline/up", {
	    method: 'PUT'
	});
    }
    if (e.id === "incline_down") {
	fetch("/incline/down", {
	    method: 'PUT'
	});
    }

    if (e.id === "interval_01") {
	document.getElementById("interval_01").style.backgroundColor = "#00C8DC";
	document.getElementById("interval_05").style.backgroundColor = "#008CBA";
	document.getElementById("interval_10").style.backgroundColor = "#008CBA";
	fetch("/speed/intervall/1", {
	    method: 'PUT'
	});
    }
    if (e.id === "interval_05") {
	document.getElementById("interval_01").style.backgroundColor = "#008CBA";
	document.getElementById("interval_05").style.backgroundColor = "#00C8DC";
	document.getElementById("interval_10").style.backgroundColor = "#008CBA";
	fetch("/speed/intervall/2", {
	    method: 'PUT'
	});
    }
    if (e.id === "interval_10") {
	document.getElementById("interval_01").style.backgroundColor = "#008CBA";
	document.getElementById("interval_05").style.backgroundColor = "#008CBA";
	document.getElementById("interval_10").style.backgroundColor = "#00C8DC";
	fetch("/speed/intervall/3", {
	    method: 'PUT'
	});
    }
}

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
	if (this.readyState == 4 && this.status == 200) {
	    document.getElementById("speed").innerHTML = this.responseText;
	}
    };
    xhttp.open("GET", "/speed", true);
    xhttp.send();
}, 1000);

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
	if (this.readyState == 4 && this.status == 200) {
	    document.getElementById("distance").innerHTML = this.responseText;
	}
    };
    xhttp.open("GET", "/distance", true);
    xhttp.send();
}, 1000);

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
	if (this.readyState == 4 && this.status == 200) {
	    document.getElementById("incline").innerHTML = this.responseText;
	}
    };
    xhttp.open("GET", "/incline", true);
    xhttp.send();
}, 1000);

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
	if (this.readyState == 4 && this.status == 200) {
	    document.getElementById("elevation").innerHTML = this.responseText;
	}
    };
    xhttp.open("GET", "/elevation", true);
    xhttp.send();
}, 1000);

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
	if (this.readyState == 4 && this.status == 200) {
	    document.getElementById("hour").innerHTML = this.responseText;
	}
    };
    xhttp.open("GET", "/hour", true);
    xhttp.send();
}, 1000);

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
	if (this.readyState == 4 && this.status == 200) {
	    document.getElementById("minute").innerHTML = this.responseText;
	}
    };
    xhttp.open("GET", "/minute", true);
    xhttp.send();
}, 1000);

setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
	if (this.readyState == 4 && this.status == 200) {
	    document.getElementById("second").innerHTML = this.responseText;
	}
    };
    xhttp.open("GET", "/second", true);
    xhttp.send();
}, 1000);
