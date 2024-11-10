#include <WiFi.h>
#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESP32Servo.h>
#include <ESP32-TWAI-CAN.hpp>

DNSServer dnsServer;
const char* ssid = "ankle";
const char* password = "WeBotsAnkle";

AsyncWebServer server(80);

Servo LEFTServo;  
Servo RIGHTServo;

int ServoLEFTPin = 18;   
int ServoRIGHTPin = 19;

volatile int joystick1X = 0; // Joystick 1 - Horizontal
volatile int joystick1Y = 0; // Joystick 1 - Vertical
volatile int joystick2Y = 0; // Joystick 2 - Vertical (only)

#define CAN_TX 5
#define CAN_RX 4

const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
<html>
<head>
  <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
  <title>Dual Joysticks</title>
  <style>
    .joystick-container {
      display: block; /* Stacks the containers vertically */
      margin: 20px auto; /* Centers the joystick horizontally and adds vertical spacing */
      text-align: center; /* Ensures the canvas is centered within the container */
    }
    /*canvas {
      border: 1px solid #ccc;  Optional: Adds a border for visual distinction 
    }*/

  </style>
</head>
<body>
  <div class="joystick-container" id="joy1Div">
    <canvas id="joystick1" width="450" height="450"></canvas>
  </div>
  <div class="joystick-container" id="joy2Div">
    <canvas id="joystick2" width="450" height="450"></canvas>
  </div>

  <script type="text/javascript">
    function createJoystick(containerId) {
  var JoyStick = function (container, parameters) {
    parameters = parameters || {};
    var title = parameters.title || "joystick",
        width = parameters.width || 0,
        height = parameters.height || 0,
        internalFillColor = parameters.internalFillColor || "#00AA00",
        internalLineWidth = parameters.internalLineWidth || 2,
        internalStrokeColor = parameters.internalStrokeColor || "#003300",
        externalLineWidth = parameters.externalLineWidth || 2,
        externalStrokeColor = parameters.externalStrokeColor || "#008000",
        autoReturnToCenter = parameters.autoReturnToCenter !== false;

    var objContainer = document.getElementById(container);
    var canvas = objContainer.querySelector('canvas');
    var context = canvas.getContext("2d");

    var pressed = false;
    var circumference = 2 * Math.PI;
    var internalRadius = (canvas.width - (canvas.width / 2 + 10)) / 2;
    var maxMoveStick = internalRadius * 2; // Adjusted for more freedom
    var externalRadius = internalRadius + 45;
    var centerX = canvas.width / 2;
    var centerY = canvas.height / 2;
    var movedX = centerX;
    var movedY = centerY;

    // Add event listeners for touch and mouse events
    canvas.addEventListener("touchstart", onTouchStart, false);
    canvas.addEventListener("touchmove", onTouchMove, false);
    canvas.addEventListener("touchend", onTouchEnd, false);
    canvas.addEventListener("mousedown", onMouseDown, false);
    canvas.addEventListener("mousemove", onMouseMove, false);
    canvas.addEventListener("mouseup", onMouseUp, false);

    // Draw joystick components
    drawExternal();
    drawInternal();

    function drawExternal() {
      context.beginPath();
      context.arc(centerX, centerY, externalRadius, 0, circumference, false);
      context.lineWidth = externalLineWidth;
      context.strokeStyle = externalStrokeColor;
      context.stroke();
    }

    function drawInternal() {
      context.beginPath();
      context.arc(movedX, movedY, internalRadius, 0, circumference, false);
      var grd = context.createRadialGradient(movedX, movedY, 5, movedX, movedY, internalRadius);
      grd.addColorStop(0, internalFillColor);
      grd.addColorStop(1, internalStrokeColor);
      context.fillStyle = grd;
      context.fill();
      context.lineWidth = internalLineWidth;
      context.strokeStyle = internalStrokeColor;
      context.stroke();
    }

    function onTouchStart(event) {
      pressed = true;
    }

    function onTouchMove(event) {
      if (pressed) {
        event.preventDefault();
        var rect = canvas.getBoundingClientRect();
        var touchX = event.touches[0].clientX - rect.left;
        var touchY = event.touches[0].clientY - rect.top;
        constrainMovement(touchX, touchY);
      }
    }

    function onTouchEnd() {
      pressed = false;
      if (autoReturnToCenter) {
        resetJoystick();
      }
    }

    function onMouseDown(event) {
      pressed = true;
    }

    function onMouseMove(event) {
      if (pressed) {
        var rect = canvas.getBoundingClientRect();
        var mouseX = event.clientX - rect.left;
        var mouseY = event.clientY - rect.top;
        constrainMovement(mouseX, mouseY);
      }
    }

    function onMouseUp() {
      pressed = false;
      if (autoReturnToCenter) {
        resetJoystick();
      }
    }

    function constrainMovement(x, y) {
      var dx = x - centerX;
      var dy = y - centerY;
      var distance = Math.sqrt(dx * dx + dy * dy);
      var maxDistance = externalRadius - internalRadius;

      // Allow movement up to the external boundary
      if (distance > maxDistance) {
        var angle = Math.atan2(dy, dx);
        movedX = centerX + maxDistance * Math.cos(angle);
        movedY = centerY + maxDistance * Math.sin(angle);
      } else {
        movedX = x;
        movedY = y;
      }

      updateJoystick();
    }

    function updateJoystick() {
      context.clearRect(0, 0, canvas.width, canvas.height);
      drawExternal();
      drawInternal();
    }

    function resetJoystick() {
      movedX = centerX;
      movedY = centerY;
      updateJoystick();
    }

    this.GetX = function () {
      return (100 * ((movedX - centerX) / maxMoveStick)).toFixed();
    };
    this.GetY = function () {
      return (100 * ((movedY - centerY) / maxMoveStick) * -1).toFixed();
    };
  };

  return new JoyStick(containerId);
}

    // Create joystick instances
    var Joy1 = createJoystick("joy1Div");
    var Joy2 = createJoystick("joy2Div");

    // Update joystick data periodically
    setInterval(function () {
      var x1 = Joy1.GetX();
      var y1 = Joy1.GetY();
      var y2 = Joy2.GetY(); // Only vertical movement for the second joystick

      fetch(`/update?x1=${x1}&y1=${y1}&y2=${y2}`)
        .then(response => response.text())
        .then(data => console.log(data))
        .catch(error => console.error('Error:', error));
    }, 50);
  </script>
</body>
</html>
)rawliteral";

class CaptiveRequestHandler : public AsyncWebHandler {
public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request) {
        request->addInterestingHeader("ANY");
        return true;
    }

    void handleRequest(AsyncWebServerRequest *request) override {
        request->send_P(200, "text/html", index_html);
    }
};

void setupServer() {
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
        Serial.println("Client Connected");
    });

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("x1") && request->hasParam("y1") && request->hasParam("y2")) {
            joystick1X = request->getParam("x1")->value().toInt();
            joystick1Y = request->getParam("y1")->value().toInt();
            joystick2Y = request->getParam("y2")->value().toInt();

            Serial.print("Joystick 1 - X: ");
            Serial.print(joystick1X);
            Serial.print(" Y: ");
            Serial.println(joystick1Y);

            Serial.print("Joystick 2 - Y: ");
            Serial.println(joystick2Y);

            updateServos(joystick1X, joystick1Y);
            sendCANMessage(joystick2Y);
        }
        request->send(200, "text/plain", "Joystick positions received");
    });
}

void setup() {
    Serial.begin(115200);
    LEFTServo.attach(ServoLEFTPin);   
    RIGHTServo.attach(ServoRIGHTPin);
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssid, password);
    setupServer();

    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    dnsServer.start(53, "*", WiFi.softAPIP());
    server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
    server.begin();

    ESP32Can.setPins(CAN_TX, CAN_RX);
    ESP32Can.setSpeed(ESP32Can.convertSpeed(500)); // 500 kbps CAN speed
    if (ESP32Can.begin()) {
        Serial.println("CAN bus initialized.");
    } else {
        Serial.println("Failed to initialize CAN bus.");
        while (1);
    }
}

void loop() {
    dnsServer.processNextRequest();
}

void updateServos(int x, int y) {
    int VerticalServoPosition = map(y, -100, 100, 70, 110);
    int HorizontalAdjustment = map(x, -100, 100, -20, 20);

    int TargetLEFTServoPosition = constrain(VerticalServoPosition + HorizontalAdjustment, 0, 40);
    int TargetRIGHTServoPosition = constrain(VerticalServoPosition - HorizontalAdjustment, 0, 40);

    int CurrentLEFTServoPosition = LEFTServo.read();    // Get the current position of the left servo
    int CurrentRIGHTServoPosition = 180 - RIGHTServo.read();  // Get the current position of the right servo

    int StepSize = 2; // Adjust this value to control the speed 

    // Gradually move the LEFT servo towards the target position
    if (CurrentLEFTServoPosition < TargetLEFTServoPosition) {
        for (int pos = CurrentLEFTServoPosition; pos <= TargetLEFTServoPosition; pos += StepSize) {
            LEFTServo.write(pos);
            delay(15); // Adjust delay for speed control; increase for slower movement
        }
    } else if (CurrentLEFTServoPosition > TargetLEFTServoPosition) {
        for (int pos = CurrentLEFTServoPosition; pos >= TargetLEFTServoPosition; pos -= StepSize) {
            LEFTServo.write(pos);
            delay(15); // Adjust delay for speed control
        }
    }

    // Gradually move the RIGHT servo towards the target position
    if (CurrentRIGHTServoPosition < TargetRIGHTServoPosition) {
        for (int pos = CurrentRIGHTServoPosition; pos <= TargetRIGHTServoPosition; pos += StepSize) {
            RIGHTServo.write(180 - pos);  // Inverting direction for the servo
            delay(15); // Adjust delay for speed control
        }
    } else if (CurrentRIGHTServoPosition > TargetRIGHTServoPosition) {
        for (int pos = CurrentRIGHTServoPosition; pos >= TargetRIGHTServoPosition; pos -= StepSize) {
            RIGHTServo.write(180 - pos);  // Inverting direction for the servo
            delay(15); // Adjust delay for speed control
        }
    }
}

void sendCANMessage(int y) {
    CanFrame frame;
    frame.identifier = 0x02040000; // Replace with the appropriate CAN ID for motor control
    frame.extd = 1; // Extended frame
    frame.data_length_code = 8;

    // Map the Y position from -100 to 100 to a speed range, for example, -255 to 255 for motor speed control
    int speed = map(y, -100, 100, -100, 100); // Adjust the mapping as needed

    // Set direction and speed in the CAN frame data
    if (speed >= 0) {
        frame.data[0] = 0x01; // Example: 0x01 for forward direction (you may need to replace this with the appropriate value for your SPARK MAX)
    } else {
        frame.data[0] = 0x02; // Example: 0x02 for reverse direction (replace with the appropriate value if needed)
        speed = abs(speed);   // Ensure speed is positive for data transmission
    }

    frame.data[1] = (uint8_t)(speed & 0xFF); // Speed as an 8-bit value (low byte)
    frame.data[2] = 0;                       // Padding or additional data, if required
    frame.data[3] = 0;                       // Padding or additional data, if required
    frame.data[4] = 0;                       // Padding or additional data, if required
    frame.data[5] = 0;                       // Padding or additional data, if required
    frame.data[6] = 0;                       // Padding or additional data, if required
    frame.data[7] = 0;                       // Padding or additional data, if required

    // Send the CAN frame
    if (ESP32Can.writeFrame(frame)) {
        Serial.print("CAN frame sent. Speed: ");
        Serial.print(speed);
        if (frame.data[0] == 0x01) {
            Serial.println(" (Forward)");
        } else {
            Serial.println(" (Reverse)");
        }
    } else {
        Serial.println("Failed to send CAN frame.");
    }
}