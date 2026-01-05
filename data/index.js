

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var manualSpeed   = false;
var manualIncline = false;
var sensorMode    = 0;

const debounceTimers = {};

window.addEventListener('load', onLoad);

function onLoad(event) {
  initWebSocket();
  initButtons();
}


/**
 * @param {string} key - unique name(eg. 'speed_up')
 * @param {function} func - callback
 * @param {number} delay -  wait in ms
 */
function debounce(key, func, delay = 200) {
  clearTimeout(debounceTimers[key]);
  debounceTimers[key] = setTimeout(func, delay);
}


// ----------------------------------------------------------------------------
// WebSocket handling
// ----------------------------------------------------------------------------

function initWebSocket() {
  //console.log('Trying to open a WebSocket connection...');

  websocket = new WebSocket(gateway);

  websocket.onopen    = onOpen;
  websocket.onclose   = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  //console.log('Connection opened');
}

function onClose(event) {
  //console.log('Connection closed');
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

  //manualIncline = 0 === (data.sensor_mode & 1);
  //manualSpeed   = 0 === (data.sensor_mode & 2);
  if (data.sensor_mode === 0) {
    manualSpeed   = true;
    manualIncline = true;
  }
  else if (data.sensor_mode === 1) {
    manualSpeed   = false;
    manualIncline = true;
  }
  else if (data.sensor_mode === 2) {
    manualSpeed   = true;
    manualIncline = false;
  }
  else if (data.sensor_mode === 3) {
    manualSpeed   = false;
    manualIncline = false;
  }

  if (sensorMode != data.sensor_mode) {
    console.log("mode changed: sensorMode: " + sensorMode + ", date.sensor_mode: " + data.sensor_mode + ", manualSpeed=" + manualSpeed + ", manualIncline=" + manualIncline);

    sensorMode = data.sensor_mode;
    setManualButtonState();
  }
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

  document.getElementById('manual_speed_button').  addEventListener('click', onSensorModeChange.bind(this, 'speed'));
  document.getElementById('manual_incline_button').addEventListener('click', onSensorModeChange.bind(this, 'incline'));
}

function onInterval(e, arg) {

  if (e === "interval_01") {
    document.getElementById("interval_01").style.backgroundColor = "#00C8DC";
    document.getElementById("interval_05").style.backgroundColor = "#008CBA";
    document.getElementById("interval_10").style.backgroundColor = "#008CBA";
    debounce('sensor_interval', () => {
      websocket.send(JSON.stringify({'command': 'speed_interval', 'value': '0.1'}));
    });
  }
  if (e === "interval_05") {
    document.getElementById("interval_01").style.backgroundColor = "#008CBA";
    document.getElementById("interval_05").style.backgroundColor = "#00C8DC";
    document.getElementById("interval_10").style.backgroundColor = "#008CBA";
    debounce('sensor_interval', () => {
      websocket.send(JSON.stringify({'command': 'speed_interval', 'value': '0.5'}));
    });
  }
  if (e === "interval_10") {
    document.getElementById("interval_01").style.backgroundColor = "#008CBA";
    document.getElementById("interval_05").style.backgroundColor = "#008CBA";
    document.getElementById("interval_10").style.backgroundColor = "#00C8DC";
    debounce('sensor_interval', () => {
      websocket.send(JSON.stringify({'command': 'speed_interval', 'value': '1.0'}));
    });
  }
}

function onSpeedUp(e) {
  debounce('speed_action', () => {
    websocket.send(JSON.stringify({'command': 'speed', 'value': 'up'}));
  }, 150);
}
function onSpeedDown(e) {
  debounce('speed_action', () => {
    websocket.send(JSON.stringify({'command': 'speed', 'value': 'down'}));
  }, 150);
}
function onInclineUp(e) {
  debounce('incline_action', () => {
    websocket.send(JSON.stringify({'command': 'incline', 'value': 'up'}));
  }, 150);
}
function onInclineDown(e) {
  debounce('incline_action', () => {
    websocket.send(JSON.stringify({'command': 'incline', 'value': 'down'}));
  }, 150);
}

function setManualButtonState() {
  if (manualSpeed === false) {
    document.getElementById('manual_speed_button').classList.remove('fa-toggle-off');
    document.getElementById('manual_speed_button').classList.remove('inactive');
    document.getElementById('manual_speed_button').classList.add('fa-toggle-on');
    document.getElementById('manual_speed_button').classList.add('active');
  }
  else {  // manual mode, set inactive
    document.getElementById('manual_speed_button').classList.remove('fa-toggle-on');
    document.getElementById('manual_speed_button').classList.remove('active');
    document.getElementById('manual_speed_button').classList.add('fa-toggle-off');
    document.getElementById('manual_speed_button').classList.add('inactive');
  }

  if (manualIncline === false) {
    document.getElementById('manual_incline_button').classList.remove('fa-toggle-off');
    document.getElementById('manual_incline_button').classList.remove('inactive');
    document.getElementById('manual_incline_button').classList.add('fa-toggle-on');
    document.getElementById('manual_incline_button').classList.add('active');
  }
  else {
    document.getElementById('manual_incline_button').classList.remove('fa-toggle-on');
    document.getElementById('manual_incline_button').classList.remove('active');
    document.getElementById('manual_incline_button').classList.add('fa-toggle-off');
    document.getElementById('manual_incline_button').classList.add('inactive');
  }
}

function onSensorModeChange(e, arg) {
  if (e === 'speed') {
    manualSpeed = !manualSpeed;

    debounce('sensor_speed', () => {
      websocket.send(JSON.stringify({'command': 'sensor_mode', 'value': 'speed'}));
    });
  }
  if (e === 'incline') {
    manualIncline = !manualIncline;

    debounce('sensor_incline', () => {
      websocket.send(JSON.stringify({'command': 'sensor_mode', 'value': 'incline'}));
    });
  }
//  setManualButtonState();
}

