

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var manualSpeed = true;
var manualIncline = true;

window.addEventListener('load', onLoad);

function onLoad(event) {
  initWebSocket();
  initButtons();
  manualAutoToggle();
}


function toggleStopwatch() {

  document.getElementById('stopwatch').style.visibility = 'visible';
  document.getElementById('stopwatch').style.visibility = 'hidden';

}

function manualAutoToggle() {
  var speedStyle   = document.getElementById('toggle_manual_speed').style,
      speedToggle = false,
      speedC1 = '#059e8a',
      speedC2 = '#24ffe2';

  var inclineStyle = document.getElementById('toggle_manual_incline').style,
      inclineToggle = false,
      inclineC1 = '#00add6',
      inclineC2 = '#24ffe2';

  setInterval(function() {
    if (!manualSpeed) { // toggle icon color in "manual mode"
      speedStyle.color = speedToggle ? speedC1 : speedC2;
      speedToggle = !speedToggle;
    }
    if (!manualIncline) {
      inclineStyle.color = inclineToggle ? inclineC1 : inclineC2;
      inclineToggle = !inclineToggle;
    }
  }, 1000);
}

// style="color:#059e8a;" -> default manual speed    auto: fade to #24ffe2
// style="color:#00add6;" -> default manual incline  auto: fade to #24ffe2



// ----------------------------------------------------------------------------
// WebSocket handling
// ----------------------------------------------------------------------------

function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');

  websocket = new WebSocket(gateway);

  websocket.onopen    = onOpen;
  websocket.onclose   = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log('Connection opened');
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  let data = JSON.parse(event.data);

  document.getElementById('speed')    .innerHTML = data.speed.toFixed(1);
  document.getElementById('distance') .innerHTML = data.distance.toFixed(2);
  document.getElementById('incline')  .innerHTML = data.incline.toFixed(1);
  document.getElementById('elevation').innerHTML = data.elevation.toFixed(1);
  document.getElementById('hour')     .innerHTML = data.hour;
	document.getElementById('minute')   .innerHTML = data.minute;
  document.getElementById('second')   .innerHTML = data.second;

	data.sensor_mode
}

// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButtons() {
  document.getElementById('interval_01').addEventListener('click', onInterval.bind(this, 'interval_01'));
  document.getElementById('interval_05').addEventListener('click', onInterval.bind(this, 'interval_05'));
  document.getElementById('interval_10').addEventListener('click', onInterval.bind(this, 'interval_10'));

  document.getElementById('speed_up')    .addEventListener('click', onSpeedUp);
  document.getElementById('speed_down')  .addEventListener('click', onSpeedDown);
  document.getElementById('incline_up')  .addEventListener('click', onInclineUp);
  document.getElementById('incline_down').addEventListener('click', onInclineDown);

  // add handler to toggle manual speed/incline by clicking on the runner (kmh) or incline (%) icon
  document.getElementById('toggle_manual_speed').addEventListener('click', onSensorModeChange.bind(this, 'speed'));
  document.getElementById('toggle_manual_incline').addEventListener('click', onSensorModeChange.bind(this, 'incline'));
  
}

function onInterval(e, arg) {

  if (e === "interval_01") {
    document.getElementById("interval_01").style.backgroundColor = "#00C8DC";
    document.getElementById("interval_05").style.backgroundColor = "#008CBA";
    document.getElementById("interval_10").style.backgroundColor = "#008CBA";
    websocket.send(JSON.stringify({'command': 'speed_interval',
				   'value': '0.1'}));
  }
  if (e === "interval_05") {
    document.getElementById("interval_01").style.backgroundColor = "#008CBA";
    document.getElementById("interval_05").style.backgroundColor = "#00C8DC";
    document.getElementById("interval_10").style.backgroundColor = "#008CBA";
    websocket.send(JSON.stringify({'command': 'speed_interval',
				   'value': '0.5'}));
  }
  if (e === "interval_10") {
    document.getElementById("interval_01").style.backgroundColor = "#008CBA";
    document.getElementById("interval_05").style.backgroundColor = "#008CBA";
    document.getElementById("interval_10").style.backgroundColor = "#00C8DC";
    websocket.send(JSON.stringify({'command': 'speed_interval',
				   'value': '1.0'}));
  }
}

function onSpeedUp(e) {
  websocket.send(JSON.stringify({'command': 'speed',
				 'value': 'up'}));
}
function onSpeedDown(e) {
  websocket.send(JSON.stringify({'command': 'speed',
				 'value': 'down'}));
}
function onInclineUp(e) {
  websocket.send(JSON.stringify({'command': 'incline',
				 'value': 'up'}));
}
function onInclineDown(e) {
  websocket.send(JSON.stringify({'command': 'incline',
				 'value': 'down'}));
}

function onSensorModeChange(e, arg) {

  if (e === 'speed') {
    manualSpeed = !manualSpeed;
    //document.getElementById('toggle_manual_speed').style.color = manualSpeed ? ...
    websocket.send(JSON.stringify({'command': 'sensor_mode',
				   'value': 'speed'}));
  }
  if (e === 'incline') {
    manualIncline = !manualIncline;
    websocket.send(JSON.stringify({'command': 'sensor_mode',
				   'value': 'incline'}));
  }
}
