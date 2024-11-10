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
    <title>Joy</title>
  </head>
  <body>
    <div class="row">
      <div class="columnLateral">
        <div id="joy1Div" style="width: 650px; height: 650px; margin: 175px">
          <canvas id="joystick1" width="650" height="650"></canvas>
        </div>
      </div>
      <div class="columnLateral">
        <div id="joy2Div" style="width: 650px; height: 650px; margin: 175px">
          <canvas id="joystick2" width="650" height="650"></canvas>
        </div>
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

      var Joy2 = createJoystick("joy2Div");
      var joy2Y = document.getElementById("joy2Y");
      
      

      setInterval(function () {
  var x1 = Joy1.GetX();
  var y1 = Joy1.GetY();
  var y2 = Joy2.GetY();
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
    int VerticalServoPosition = map(y, -100, 100, 60, 120);
    int HorizontalAdjustment = map(x, -100, 100, -20, 20);

    int TargetLEFTServoPosition = constrain(VerticalServoPosition + HorizontalAdjustment, 0, 40);
    int TargetRIGHTServoPosition = constrain(VerticalServoPosition - HorizontalAdjustment, 0, 40);

    LEFTServo.write(TargetLEFTServoPosition);
    RIGHTServo.write(180 - TargetRIGHTServoPosition);
}

void sendCANMessage(int y) {
    CanFrame frame;
    frame.identifier = 0x02040000; // Replace with appropriate CAN ID
    frame.extd = 1; // Extended frame
    frame.data_length_code = 8;

    frame.data[0] = (uint8_t)(y & 0xFF); // Low byte of joystick Y
    frame.data[1] = (uint8_t)((y >> 8) & 0xFF); // High byte of joystick Y
    for (int i = 2; i < 8; i++) frame.data[i] = 0;

    if (ESP32Can.writeFrame(frame)) {
        Serial.println("CAN frame sent.");
    } else {
        Serial.println("Failed to send CAN frame.");
    }
}