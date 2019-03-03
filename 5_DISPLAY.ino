class oled
{
    unsigned long lastNTPupdate = 0;
    // This array keeps function pointers to all frames
    // frames are the single views that slide in
    FrameCallback frames[5] = { connectionState, sensorState, sensorOne, sensorTwo, sensorThree };

    // how many frames are there?
    int frameCount = 5;

    // Overlays are statically drawn on top of a frame eg. a clock
    OverlayCallback overlays[1] = { clockOverlay };
    int overlaysCount = 1;

  public:
    String t;
    bool dispEnabled = 0;
    int address;

    bool senOK = true;
    bool actOK = true;
    bool indOK = true;
    bool wlanOK = true;
    bool mqttOK = false;

    oled() {
    }

    void dispUpdate() {
      if (dispEnabled == 1) {
        DBG_PRINTLN("UPDATE: Display");
        /*        showDispClear();
                showDispTime(t);
                showDispIP(WiFi.localIP().toString());
                showDispWlan();
                showDispMqtt();
                showDispLines();
                showDispSen();
                showDispAct();
                showDispInd();
                showDispDisplay();*/
      }
    }

    void change(int dispAddress, bool is_enabled) {
      if (is_enabled == 1 && dispAddress != 0) {
        DBG_PRINT("CHANGE: Display switched on: ");
        DBG_PRINTLN(dispAddress);
        address = dispAddress;
        // The ESP is capable of rendering 60fps in 80Mhz mode
        // but that won't give you much time for anything else
        // run it in 160Mhz mode or just set it to 30 fps
        ui.setTargetFPS(DISPLAY_FPS);

        // Customize the active and inactive symbol
        ui.setActiveSymbol(activeSymbol);
        ui.setInactiveSymbol(inactiveSymbol);

        // You can change this to
        // TOP, LEFT, BOTTOM, RIGHT
        ui.setIndicatorPosition(TOP);

        // Defines where the first frame is located in the bar.
        ui.setIndicatorDirection(LEFT_RIGHT);

        // You can change the transition that is used
        // SLIDE_LEFT, SLIDE_RIGHT, SLIDE_UP, SLIDE_DOWN
        ui.setFrameAnimation(SLIDE_LEFT);

        // Add frames
        ui.setFrames(frames, frameCount);

        // Add overlays
        ui.setOverlays(overlays, overlaysCount);

        // Initialising the UI will init the display too.
        ui.init();
        display.flipScreenVertically();
        display.displayOn();
        display.clear();
        display.display();
        dispEnabled = is_enabled;
        timeClient.begin();
        digClock();
      }
      else {
        dispEnabled = is_enabled;
      }
    }

    void digClock()
    {
      t = ""; // clear value for display
      time_t local, utc;
      if (millis() > (lastNTPupdate + NTP_INTERVAL))
      {
        cbpiEventSystem(EM_NTP);
        lastNTPupdate = millis();
      }
      unsigned long epochTime =  timeClient.getEpochTime();
      // convert received time stamp to time_t object
      // time_t local, utc;
      utc = epochTime;
      TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 0};     //Central European Summer Time
      TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 0};       //Central European Standard Time
      Timezone CE(CEST, CET );
      local = CE.toLocal(utc);
      setTime(local);
      t += hour(local);
      t += ":";
      if (minute(local) < 10) // add a zero if minute is under 10
        t += "0";
      t += minute(local);
    }

}

oledDisplay = oled();

void clockOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) {
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->setFont(ArialMT_Plain_10);
  display->drawString(10, 0, oledDisplay.t);
}

void sensorState(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  if (sensorsStatus == 0) {
    display->drawString(5, 20, "Sen: ");
    display->drawString(40, 20, String(numberOfSensors));
  }
  else display->drawString(10, 20, "Sen:Er");
}
void connectionState(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  if (oledDisplay.wlanOK) {
    display->drawXbm(3, 20, wlan_logo_width, wlan_logo_height, wlan_logo);
  }
  display->drawString(25, 20, WiFi.localIP().toString());
  if (oledDisplay.mqttOK) {
    display->drawXbm(3, 40, mqtt_logo_width, mqtt_logo_height, mqtt_logo);
  }
}

void sensorOne(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  TemperatureSensor sensorToPrint = sensors[0];
  String sensorName = sensorToPrint.sens_name;
  display->drawString(5, 10, sensorName);
  String sensorConnState = "Disconnected";
  String sensorValue = "-1";
  if ( sensorToPrint.sens_isConnected ) {
    sensorConnState = "Connected";
    sensorValue = String(sensorToPrint.sens_value);
  }
  display->setFont(ArialMT_Plain_10);
  display->drawString(5, 28, sensorConnState);
  display->setFont(ArialMT_Plain_16);
  display->drawString(5, 45, sensorValue);
}

void sensorTwo(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  TemperatureSensor sensorToPrint = sensors[1];
  String sensorName = sensorToPrint.sens_name;
  display->drawString(5, 10, sensorName);
  String sensorConnState = "Disconnected";
  String sensorValue = "-1";
  if ( sensorToPrint.sens_isConnected ) {
    sensorConnState = "Connected";
    sensorValue = String(sensorToPrint.sens_value);
  }
  display->setFont(ArialMT_Plain_10);
  display->drawString(5, 28, sensorConnState);
  display->setFont(ArialMT_Plain_16);
  display->drawString(5, 45, sensorValue);
}

void sensorThree(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_16);
  TemperatureSensor sensorToPrint = sensors[2];
  String sensorName = sensorToPrint.sens_name;
  display->drawString(5, 10, sensorName);
  String sensorConnState = "Disconnected";
  String sensorValue = "-1";
  if ( sensorToPrint.sens_isConnected ) {
    sensorConnState = "Connected";
    sensorValue = String(sensorToPrint.sens_value);
  }
  display->setFont(ArialMT_Plain_10);
  display->drawString(5, 28, sensorConnState);
  display->setFont(ArialMT_Plain_16);
  display->drawString(5, 45, sensorValue);
}


void turnDisplayOff() {
  if (oledDisplay.dispEnabled) {
    DBG_PRINTLN("Switch OLED display off");
    display.displayOff();
    oledDisplay.dispEnabled = 0;
  }
  else {
    if (oledDisplay.address != 0) {
      DBG_PRINTLN("Switch OLED display on");
      display.displayOn();
      oledDisplay.dispEnabled = 1;
    }
  }
}

void handleRequestDisplay() {
  StaticJsonBuffer<1024> jsonBuffer;
  JsonObject& displayResponse = jsonBuffer.createObject();
  displayResponse["enabled"] = 0;
  displayResponse["displayOn"] = 0x3C;
  displayResponse["enabled"] = oledDisplay.dispEnabled;
  displayResponse["updisp"] = DISP_UPDATE;
  if (oledDisplay.dispEnabled == 1) {
    displayResponse["displayOn"] = 1;
  }
  else {
    displayResponse["displayOn"] = 0;
  }

  String response;
  displayResponse.printTo(response);
  server.send(200, "application/json", response);
}

void handleRequestDisp() {
  String request = server.arg(0);
  String message;
  if (request == "isEnabled") {
    message = "0";
    if (oledDisplay.dispEnabled) {
      message = "1";
    }
    goto SendMessage;
  }
  if (request == "address") {
    message = "0x3C";
    message = String(decToHex(oledDisplay.address, 2));
    goto SendMessage;
  }
  if (request == "updisp") {
    message = DISP_UPDATE / 1000;
    goto SendMessage;
  }
SendMessage:
  server.send(200, "text/plain", message);
}

void handleSetDisp() {
  String dispAddress;
  int address;
  address = oledDisplay.address;
  for (int i = 0; i < server.args(); i++) {
    if (server.argName(i) == "enabled") {
      if (server.arg(i) == "1") {
        oledDisplay.dispEnabled = 1;
      } else {
        oledDisplay.dispEnabled = 0;
      }
    }
    if (server.argName(i) == "address")  {

      dispAddress = server.arg(i);
      dispAddress.remove(0, 2);
      char copy[4];
      dispAddress.toCharArray(copy, 4);
      address = strtol(copy, 0, 16);
    }
    yield();
  }
  oledDisplay.change(address, oledDisplay.dispEnabled);
  saveConfig();
}

void dispStartScreen()               // Show Startscreen
{
  if (oledDisplay.dispEnabled == 1 && oledDisplay.address != 0) {
    display.displayOn();
    showDispCbpi();
    showDispSTA();
    showDispDisplay();
  }
}

/* ######### Display functions ######### */
void showDispClear()              // Clear Display
{
  display.clear();
  /*  display.display();*/
}

void showDispDisplay()            // Show
{
  display.display();
}

/*void showDispVal(String value)    // Display a String value
  {
  display.drawString(value);
  display.display();
  }*/

/*void showDispVal(int value)       // Display a Int value
  {
  display.drawString(value);
  display.display();
  }
*/
/*void showDispWlan()               // Show WLAN icon
  {
  if (oledDisplay.wlanOK) {
    display.drawXbm(77, 3, 20, 20, wlan_logo);
  }
  }*/
void showDispMqtt()               // SHow MQTT icon
{
  if (oledDisplay.mqttOK) {
    display.drawXbm(50, 3, 20, 20, mqtt_logo);
  }
}

void showDispOTA(unsigned int progress, unsigned int total)               // Show OTA icon
{
  int otaStatus = progress / (total / 100);
  bool up = false;
  switch (otaStatus) {
    case 0:
    case 20:
    case 40:
    case 60:
    case 80:
    case 100:
      up = true;
      break;
    default:
      up = false;
      break;
  }
  if (up) {
    display.clear();
    display.setFont(ArialMT_Plain_10);
    display.drawString(5, 5, "OTA Update: ");
    display.drawString(20, 5, String(otaStatus));
    int xoffset = 5;
    display.drawRect( xoffset, 25, otaStatus + xoffset, 2);
    display.fillRect( xoffset, 25, otaStatus + xoffset, 2);
    if (otaStatus > 80) {
      display.drawString(5, 40, "OTA Update finished");
      display.drawString(50, 50, "... reboot");
    }
    display.display();
  }
}

void showDispOTAEr(String value)
{
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(5, 5, "OTA Update Error: ");
  display.drawString(20, 5, value);
  display.display();
}


void showDispCbpi()               // SHow CBPI icon
{
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawXbm(41, 0, 50, 50, cbpi_logo);
}

void showDispLines()              // Draw lines in the bottom
{
  display.drawLine(0, 50, 128, 50);
  display.drawLine(42, 50, 42, 64);
  display.drawLine(84, 50, 84, 64);
}

void showDispSen()                // Show Sensor status on the left
{
  display.setFont(ArialMT_Plain_10);
  if (sensorsStatus == 0) {
    display.drawString(3, 55, "Sen: ");
    display.drawString(8, 55, String(numberOfSensors));
  }
  else display.drawString(3, 55, "Sen:Er");
}
void showDispAct()                // Show actor status in the mid
{
  display.setFont(ArialMT_Plain_10);
  if (actorsStatus == 0) {
    display.drawString(45, 55, "Act: ");
    display.drawString(48, 55, String(numberOfActors));
  }
  else display.drawString(45, 55, "Act:Er");
}
void showDispInd()                // Show InductionCooker status on the right
{
  display.setFont(ArialMT_Plain_10);
  if (inductionStatus == 0) display.drawString(87, 55 , "Ind:ok");
  else display.drawString(87, 55 , "Ind:Er");
}

void showDispTime(String value)   // Show time value in the upper left with fontsize 2
{
  display.setFont(ArialMT_Plain_10);
  display.drawString(5, 5, value);
}

void showDispIP(String value)      // Show IP address under time value with fontsize 1
{
  display.setFont(ArialMT_Plain_10);
  display.drawString(35, 20, value);
}

void showDispSet(String value)    // Show current station mode
{
  display.clear();
  display.setFont(ArialMT_Plain_10);
  display.drawString(5, 30, value);
  display.display();
}

void showDispSet()                // Show current station mode
{
  display.setFont(ArialMT_Plain_10);
  display.drawString(1, 54 , "SET");
  display.drawString(4, 54 , WiFi.localIP().toString());
  display.display();
}

void showDispSTA()               // Show AP mode
{
  display.setFont(ArialMT_Plain_10);
  display.drawString(8, 54 , "STA");
  display.drawString(12, 54 , WiFi.localIP().toString());
  display.display();

}
