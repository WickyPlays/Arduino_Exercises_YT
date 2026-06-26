const OBSWebSocket = require("obs-websocket-js").default;
const { SerialPort, ReadlineParser } = require("serialport");

const obs = new OBSWebSocket();

let serialPort = null;
let parser = null;
let isRecording = false;
let initialSynced = false;
let obsConnectedFlag = false;

let OBS_WS_IP = "ws://192.168.1.7:4455"

function sendToSerial(line) {
  if (!serialPort || !serialPort.writable) return;
  console.log("Sending to serial:", line);

  serialPort.write(line + "\n", (err) => {
    if (err) console.error("Serial write error:", err);
  });
}

function findSerialPort() {
  let portName;
  if (process.platform === "win32") {
    portName = "COM7";
  } else if (process.platform === "darwin") {
    portName = "/dev/tty.usbmodem101";
  } else {
    portName = "/dev/ttyUSB0";
  }
  return portName;
}

async function initialSyncSequence() {
  if (!serialPort || !serialPort.isOpen || !obsConnectedFlag || initialSynced)
    return;
  try {
    const rec = await obs
      .call("GetRecordStatus")
      .catch(() => ({ outputActive: false }));
    isRecording = rec.outputActive;
    console.log(
      `Initial recording status: ${rec.outputActive ? "ON AIR" : "OFF"}`,
    );
    
    // Send initial state to serial
    if (rec.outputActive) {
      sendToSerial("ON");
    } else {
      sendToSerial("OFF");
    }
    
    initialSynced = true;
  } catch (e) {
    console.error("Initial sync error:", e);
    initialSynced = true;
  }
}

async function connectSerial() {
  if (serialPort && serialPort.isOpen) {
    try {
      serialPort.close();
    } catch (e) {}
    serialPort = null;
    parser = null;
  }

  const portPath = findSerialPort();

  try {
    serialPort = new SerialPort({
      path: portPath,
      baudRate: 9600,
      autoOpen: false,
    });

    serialPort.open((err) => {
      if (err) {
        console.error("Serial port open error:", err);
        return;
      }

      parser = serialPort.pipe(new ReadlineParser({ delimiter: "\r\n" }));
      console.log(`Serial port opened: ${portPath}`);

      attachSerialHandlers();
      setTimeout(() => initialSyncSequence(), 120);
    });
  } catch (e) {
    console.error("Serial port error:", e);
  }
}

function attachSerialHandlers() {
  if (!parser) return;

  parser.on("data", async (line) => {
    console.log("Arduino:", line);
  });

  serialPort.on("open", () => {
    console.log("Serial port open event");
    setTimeout(() => initialSyncSequence(), 120);
  });

  serialPort.on("close", () => {
    console.log("Serial port closed");
  });

  serialPort.on("error", (err) => {
    console.error("Serial port error:", err);
  });
}

async function connectOBS() {
  try {
    await obs.connect(OBS_WS_IP);
    obsConnectedFlag = true;
    console.log("OBS connected");

    const rec = await obs
      .call("GetRecordStatus")
      .catch(() => ({ outputActive: false }));
    isRecording = rec.outputActive;

    await initialSyncSequence();
    console.log(`Recording status: ${rec.outputActive ? "ON AIR" : "OFF"}`);
  } catch (e) {
    console.error("OBS connection error:", e);
    obsConnectedFlag = false;

    setTimeout(() => {
      console.log("Reconnecting to OBS...");
      connectOBS();
    }, 5000);
  }
}

obs.on("RecordStateChanged", (d) => {
  isRecording = d.outputActive;
  console.log(`Recording: ${d.outputActive ? "ON AIR" : "OFF"}`);
  if (d.outputActive) {
    sendToSerial("ON");
  } else {
    sendToSerial("OFF");
  }
});

obs.on("ConnectionClosed", () => {
  console.log("OBS connection closed");
  obsConnectedFlag = false;
  initialSynced = false;
  setTimeout(() => {
    console.log("Reconnecting to OBS...");
    connectOBS();
  }, 5000);
});

console.log("Starting OBS Recording Controller");
console.log(`OBS target: ${OBS_WS_IP}`);
console.log(`Serial port: ${findSerialPort()}`);

connectSerial();
connectOBS();

process.on("SIGINT", () => {
  console.log("Shutting down...");
  if (serialPort && serialPort.isOpen) {
    serialPort.close();
  }
  process.exit();
});