#include <WiFi.h>
#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <DNSServer.h>
#include <ESP32Servo.h>

DNSServer dnsServer;

// Replace with your network credentials
const char* ssid = "ankle";
const char* password = "WeBotsAnkle";

// Set web server port number to 80
AsyncWebServer server(80);

Servo LEFTServo;  
Servo RIGHTServo;

int ServoLEFTPin = 18;   
int ServoRIGHTPin = 19;

volatile int joystickX = 0; // Horizontal
volatile int joystickY = 0; // Vertical



const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
<html>
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
    <title>Joy</title>
  </head>
  <body>
    <div class="row">
      <div class="columnLateral">
        <div id="joy1Div" style="width: 650px; height: 650px; margin: 175px">
          <canvas id="joystick" width="650" height="650"></canvas>
        </div>

      </div>
  
    <script type="text/javascript">
    var JoyStick = function (container, parameters) {
  parameters = parameters || {};
  var title =
      typeof parameters.title === "undefined" ? "joystick" : parameters.title,
    width = typeof parameters.width === "undefined" ? 0 : parameters.width,
    height = typeof parameters.height === "undefined" ? 0 : parameters.height,
    internalFillColor =
      typeof parameters.internalFillColor === "undefined"
        ? "#00AA00"
        : parameters.internalFillColor,
    internalLineWidth =
      typeof parameters.internalLineWidth === "undefined"
        ? 2
        : parameters.internalLineWidth,
    internalStrokeColor =
      typeof parameters.internalStrokeColor === "undefined"
        ? "#003300"
        : parameters.internalStrokeColor,
    externalLineWidth =
      typeof parameters.externalLineWidth === "undefined"
        ? 2
        : parameters.externalLineWidth,
    externalStrokeColor =
      typeof parameters.externalStrokeColor === "undefined"
        ? "#008000"
        : parameters.externalStrokeColor,
    autoReturnToCenter =
      typeof parameters.autoReturnToCenter === "undefined"
        ? true
        : parameters.autoReturnToCenter;

  // Create Canvas element and add it in the Container object
  var objContainer = document.getElementById(container);
  var canvas = document.createElement("canvas");
  canvas.id = title;
  if (width === 0) {
    width = objContainer.clientWidth;
  }
  if (height === 0) {
    height = objContainer.clientHeight;
  }
  canvas.width = width;
  canvas.height = height;
  objContainer.appendChild(canvas);
  var context = canvas.getContext("2d");

  var pressed = 0; // Bool - 1=Yes - 0=No
  var circumference = 2 * Math.PI;
  var internalRadius = (canvas.width - (canvas.width / 2 + 10)) / 2;
  var maxMoveStick = internalRadius + 5;
  var externalRadius = internalRadius + 30;
  var centerX = canvas.width / 2;
  var centerY = canvas.height / 2;
  var directionHorizontalLimitPos = canvas.width / 10;
  var directionHorizontalLimitNeg = directionHorizontalLimitPos * -1;
  var directionVerticalLimitPos = canvas.height / 10;
  var directionVerticalLimitNeg = directionVerticalLimitPos * -1;
  // Used to save current position of stick
  var movedX = centerX;
  var movedY = centerY;

  // Check if the device support the touch or not
  if ("ontouchstart" in document.documentElement) {
    canvas.addEventListener("touchstart", onTouchStart, false);
    canvas.addEventListener("touchmove", onTouchMove, false);
    canvas.addEventListener("touchend", onTouchEnd, false);
  } else {
    canvas.addEventListener("mousedown", onMouseDown, false);
    canvas.addEventListener("mousemove", onMouseMove, false);
    canvas.addEventListener("mouseup", onMouseUp, false);
  }
  // Draw the object
  drawExternal();
  drawInternal();

  /******************************************************
   * Private methods
   *****************************************************/

  /**
   * @desc Draw the external circle used as reference position
   */
  function drawExternal() {
    context.beginPath();
    context.arc(centerX, centerY, externalRadius, 0, circumference, false);
    context.lineWidth = externalLineWidth;
    context.strokeStyle = externalStrokeColor;
    context.stroke();
  }

  /**
   * @desc Draw the internal stick in the current position the user have moved it
   */
  function drawInternal() {
    context.beginPath();
    if (movedX < internalRadius) {
      movedX = maxMoveStick;
    }
    if (movedX + internalRadius > canvas.width) {
      movedX = canvas.width - maxMoveStick;
    }
    if (movedY < internalRadius) {
      movedY = maxMoveStick;
    }
    if (movedY + internalRadius > canvas.height) {
      movedY = canvas.height - maxMoveStick;
    }
    context.arc(movedX, movedY, internalRadius, 0, circumference, false);
    // create radial gradient
    var grd = context.createRadialGradient(
      centerX,
      centerY,
      5,
      centerX,
      centerY,
      200
    );
    // Light color
    grd.addColorStop(0, internalFillColor);
    // Dark color
    grd.addColorStop(1, internalStrokeColor);
    context.fillStyle = grd;
    context.fill();
    context.lineWidth = internalLineWidth;
    context.strokeStyle = internalStrokeColor;
    context.stroke();
  }

  /**
   * @desc Events for manage touch
   */
  function onTouchStart(event) {
    pressed = 1;
  }

  function onTouchMove(event) {
    // Prevent the browser from doing its default thing (scroll, zoom)
    event.preventDefault();
    if (pressed === 1 && event.targetTouches[0].target === canvas) {
      movedX = event.targetTouches[0].pageX;
      movedY = event.targetTouches[0].pageY;
      // Manage offset
      if (canvas.offsetParent.tagName.toUpperCase() === "BODY") {
        movedX -= canvas.offsetLeft;
        movedY -= canvas.offsetTop;
      } else {
        movedX -= canvas.offsetParent.offsetLeft;
        movedY -= canvas.offsetParent.offsetTop;
      }
      // Delete canvas
      context.clearRect(0, 0, canvas.width, canvas.height);
      // Redraw object
      drawExternal();
      drawInternal();
    }
  }

  function onTouchEnd(event) {
    pressed = 0;
    // If required reset position store variable
    if (autoReturnToCenter) {
      movedX = centerX;
      movedY = centerY;
    }
    // Delete canvas
    context.clearRect(0, 0, canvas.width, canvas.height);
    // Redraw object
    drawExternal();
    drawInternal();
    //canvas.unbind('touchmove');
  }

  /**
   * @desc Events for manage mouse
   */
  function onMouseDown(event) {
    pressed = 1;
  }

  function onMouseMove(event) {
    if (pressed === 1) {
      movedX = event.pageX;
      movedY = event.pageY;
      // Manage offset
      if (canvas.offsetParent.tagName.toUpperCase() === "BODY") {
        movedX -= canvas.offsetLeft;
        movedY -= canvas.offsetTop;
      } else {
        movedX -= canvas.offsetParent.offsetLeft;
        movedY -= canvas.offsetParent.offsetTop;
      }
      // Delete canvas
      context.clearRect(0, 0, canvas.width, canvas.height);
      // Redraw object
      drawExternal();
      drawInternal();
    }
  }

  function onMouseUp(event) {
    pressed = 0;
    // If required reset position store variable
    if (autoReturnToCenter) {
      movedX = centerX;
      movedY = centerY;
    }
    // Delete canvas
    context.clearRect(0, 0, canvas.width, canvas.height);
    // Redraw object
    drawExternal();
    drawInternal();
    //canvas.unbind('mousemove');
  }

  /******************************************************
   * Public methods
   *****************************************************/

  /**
   * @desc The width of canvas
   * @return Number of pixel width
   */
  this.GetWidth = function () {
    return canvas.width;
  };

  /**
   * @desc The height of canvas
   * @return Number of pixel height
   */
  this.GetHeight = function () {
    return canvas.height;
  };

  /**
   * @desc The X position of the cursor relative to the canvas that contains it and to its dimensions
   * @return Number that indicate relative position
   */
  this.GetPosX = function () {
    return movedX;
  };

  /**
   * @desc The Y position of the cursor relative to the canvas that contains it and to its dimensions
   * @return Number that indicate relative position
   */
  this.GetPosY = function () {
    return movedY;
  };

  /**
   * @desc Normalizzed value of X move of stick
   * @return Integer from -100 to +100
   */
  this.GetX = function () {
    return (100 * ((movedX - centerX) / maxMoveStick)).toFixed();
  };

  /**
   * @desc Normalizzed value of Y move of stick
   * @return Integer from -100 to +100
   */
  this.GetY = function () {
    return (100 * ((movedY - centerY) / maxMoveStick) * -1).toFixed();
  };

  /**
   * @desc Get the direction of the cursor as a string that indicates the cardinal points where this is oriented
   * @return String of cardinal point N, NE, E, SE, S, SW, W, NW and C when it is placed in the center
   */
  this.GetDir = function () {
    var result = "";
    var orizontal = movedX - centerX;
    var vertical = movedY - centerY;

    if (
      vertical >= directionVerticalLimitNeg &&
      vertical <= directionVerticalLimitPos
    ) {
      result = "C";
    }
    if (vertical < directionVerticalLimitNeg) {
      result = "N";
    }
    if (vertical > directionVerticalLimitPos) {
      result = "S";
    }

    if (orizontal < directionHorizontalLimitNeg) {
      if (result === "C") {
        result = "W";
      } else {
        result += "W";
      }
    }
    if (orizontal > directionHorizontalLimitPos) {
      if (result === "C") {
        result = "E";
      } else {
        result += "E";
      }
    }

    return result;
  };
};
      // Create JoyStick object into the DIV 'joy1Div'
      var Joy1 = new JoyStick("joy1Div");
      var joy1X = document.getElementById("joy1X");
      var joy1Y = document.getElementById("joy1Y");

      setInterval(function () {
  var x = Joy1.GetX();
  var y = Joy1.GetY();
  fetch(`/update?x=${x}&y=${y}`)
    .then(response => response.text())
    .then(data => console.log(data))
    .catch(error => console.error('Error:', error));
    }, 50);

    </script>
  </body>
</html>
)rawliteral";

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;


class CaptiveRequestHandler : public AsyncWebHandler {
public:
    CaptiveRequestHandler() {}
    virtual ~CaptiveRequestHandler() {}

    bool canHandle(AsyncWebServerRequest *request)  {
        request->addInterestingHeader("ANY");
        return true;
    }

    void handleRequest(AsyncWebServerRequest *request) override {
        request->send_P(200, "text/html", index_html);
    }
};
void setupServer()
{
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send_P(200, "text/html", index_html); 
              Serial.println("Client Connected"); 
              }
              );
    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (request->hasParam("x") && request->hasParam("y")) {
      joystickX = request->getParam("x")->value().toInt();
      joystickY = request->getParam("y")->value().toInt();
      
      Serial.print("Joystick X: ");
      Serial.print(joystickX);
      Serial.print(" Y: ");
      Serial.println(joystickY);

      // Immediately update servo positions based on the new joystick values
      updateServos(joystickX, joystickY);
    }
  request->send(200, "text/plain", "Joystick position received");
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



  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  dnsServer.start(53, "*", WiFi.softAPIP());
  server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);
  server.begin();
}

void loop(){
 dnsServer.processNextRequest();
}
void updateServos(int x, int y) {
  // Map joystick values to servo positions
  int VerticalServoPosition = map(y, -100, 100, 60, 120); // Assuming joystick Y goes from -100 to 100
  int HorizontalAdjustment = map(x, -100, 100, -20, 20); // Assuming joystick X goes from -100 to 100
  
  int TargetLEFTServoPosition = constrain(VerticalServoPosition + HorizontalAdjustment, 0, 40);
  int TargetRIGHTServoPosition = constrain(VerticalServoPosition - HorizontalAdjustment, 0, 40);

  int CurrentLEFTServoPosition = LEFTServo.read();    // Read the current position of the servo
  int CurrentRIGHTServoPosition = RIGHTServo.read();  // Read the current position of the servo

  int StepSize = 1; // Adjust this value to control the speed (smaller steps will be slower)
  
  // Gradually move the LEFT servo
  if (CurrentLEFTServoPosition < TargetLEFTServoPosition) {
    for (int pos = CurrentLEFTServoPosition; pos <= TargetLEFTServoPosition; pos += StepSize) {
      LEFTServo.write(pos);
      delay(15); // Adjust the delay for slower or faster movement
    }
  } else {
    for (int pos = CurrentLEFTServoPosition; pos >= TargetLEFTServoPosition; pos -= StepSize) {
      LEFTServo.write(pos);
      delay(15); // Adjust the delay for slower or faster movement
    }
  }

  // Gradually move the RIGHT servo
  if (CurrentRIGHTServoPosition < TargetRIGHTServoPosition) {
    for (int pos = CurrentRIGHTServoPosition; pos <= TargetRIGHTServoPosition; pos += StepSize) {
      RIGHTServo.write(180 - pos);  // Inverting direction for the servo
      delay(15); // Adjust the delay for slower or faster movement
    }
  } else {
    for (int pos = CurrentRIGHTServoPosition; pos >= TargetRIGHTServoPosition; pos -= StepSize) {
      RIGHTServo.write(180 - pos);  // Inverting direction for the servo
      delay(15); // Adjust the delay for slower or faster movement
    }
  }

  // Debug output to serial monitor
  Serial.print("LEFTServoPosition: ");
  Serial.print(TargetLEFTServoPosition);
  Serial.print(" RIGHTServoPosition: ");
  Serial.println(TargetRIGHTServoPosition);
}