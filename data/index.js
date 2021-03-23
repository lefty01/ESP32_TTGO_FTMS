

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

window.addEventListener('load', onLoad);

function onLoad(event) {
  initWebSocket();
  initButtons();
}


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
}

// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButtons() {
  document.getElementById('interval_01').addEventListener('click', onInterval.bind(this, 'interval_01'));
  document.getElementById('interval_05').addEventListener('click', onInterval.bind(this, 'interval_05'));
  document.getElementById('interval_10').addEventListener('click', onInterval.bind(this, 'interval_10'));
  // root.addEventListener('click', myPrettyHandler.bind(null, event, arg1, ... ));

  document.getElementById('speed_up')    .addEventListener('click', onSpeedUp);
  document.getElementById('speed_down')  .addEventListener('click', onSpeedDown);
  document.getElementById('incline_up')  .addEventListener('click', onInclineUp);
  document.getElementById('incline_down').addEventListener('click', onInclineDown);
}

function onInterval(e, arg) {
  //e.preventDefault(); // ??
  //console.log('onInterval');
  console.log(e);
  console.log(arg);

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

// todo: callback with argument...
// root.addEventListener('click', myPrettyHandler.bind(null, event, arg1, ... ));
//this.setup = function () {
//  this.on('tweet', this.handleStreamEvent.bind(this, 'tweet'));
//  this.on('retweet', this.handleStreamEvent.bind(this, 'retweet'));
//};
