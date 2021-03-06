void setup()
{
  Serial.begin(115200);

  gEM.addListener(EventManager::cbpiEventSystem, listenerSystem);
  gEM.addListener(EventManager::cbpiEventSensors, listenerSensors);
  gEM.addListener(EventManager::cbpiEventActors, listenerActors);
  gEM.addListener(EventManager::cbpiEventInduction, listenerInduction);

  // Set device name
  snprintf(mqtt_clientid, 25, "ESP8266-%08X", mqtt_chip_key);
  WiFi.hostname(mqtt_clientid);

  // Start sensors
  DS18B20.begin();

  // Load filesystem
  ESP.wdtFeed();
  if (!SPIFFS.begin())
  {
    DBG_PRINT("SPIFFS Mount failed");
  }
  else
  {
    DBG_PRINTLN("");
    Dir dir = SPIFFS.openDir("/");
    while (dir.next())
    {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      DBG_PRINT("FS File: ");
      DBG_PRINT(fileName.c_str());
      DBG_PRINT(" size: ");
      DBG_PRINTLN(formatBytes(fileSize).c_str());
    }
  }

  // Load configuration
  if (SPIFFS.exists("/config.json"))
  {
    ESP.wdtFeed();
    loadConfig();
  }

  // WiFi Manager
  ESP.wdtFeed();
  WiFiManagerParameter cstm_mqtthost("host", "MQTT broker IP", mqtthost, 16);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&cstm_mqtthost);
  wifiManager.autoConnect(mqtt_clientid);
  strcpy(mqtthost, cstm_mqtthost.getValue());

  // Save configuration
  ESP.wdtFeed();
  saveConfig();

  // Display Start Screen
  ESP.wdtFeed();
  dispStartScreen();

  // Load mDNS
  if (startMDNS)
  {
    ESP.wdtFeed();
    cbpiEventSystem(EM_MDNSET);
  }

  // Init Arduino Over The Air
  if (startOTA)
  {
    ESP.wdtFeed();
    setupOTA();
  }

  // Start MQTT
  client.setServer(mqtthost, 1883);
  client.setCallback(mqttcallback);

  // Start Webserver
  ESP.wdtFeed();
  setupServer();

  ESP.wdtFeed();
  cbpiEventSystem(EM_NTP);    // NTP handle
  cbpiEventSystem(EM_MDNS);   // MDNS handle
  cbpiEventSystem(EM_WLAN);   // Check WLAN
  cbpiEventSystem(EM_MQTT);   // Check MQTT
  cbpiEventSystem(EM_DISPUP); // Update display

  while (gEM.getNumEventsInQueue()) // Eventmanager process all queued events
  {
    gEM.processEvent();
  }
}

void setupServer()
{
  server.on("/", handleRoot);
 /* server.on("/setupActor", handleSetActor);       // Einstellen der Aktoren*/
  server.on("/setupSensor", handleSetSensor);     // Einstellen der Sensoren
  server.on("/reqSensors", handleRequestSensors); // Liste der Sensoren ausgeben
  /*server.on("/reqActors", handleRequestActors);   // Liste der Aktoren ausgeben*/
  /*server.on("/reqInduction", handleRequestInduction);*/
  server.on("/reqSearchSensorAdresses", handleRequestSensorAddresses);
  /*server.on("/reqPins", handlereqPins);*/
  server.on("/reqSensor", handleRequestSensor); // Infos der Sensoren für WebConfig
  /*server.on("/reqActor", handleRequestActor);   // Infos der Aktoren für WebConfig*/
  /*server.on("/reqIndu", handleRequestIndu);     // Infos der Indu für WebConfig*/
  server.on("/setSensor", handleSetSensor);     // Sensor ändern
  /*server.on("/setActor", handleSetActor);       // Aktor ändern
  server.on("/setIndu", handleSetIndu);         // Indu ändern*/
  server.on("/delSensor", handleDelSensor);     // Sensor löschen
  /*server.on("/delActor", handleDelActor);       // Aktor löschen*/
  server.on("/reboot", rebootDevice);           // reboots the whole Device
  server.on("/OTA", OTA);
  server.on("/mqttOff", turnMqttOff); // Turns off MQTT completly until reboot
  server.on("/reqDisplay", handleRequestDisplay);
  server.on("/reqDisp", handleRequestDisp); // Infos Display für WebConfig
  server.on("/setDisp", handleSetDisp);     // Display ändern
  server.on("/displayOff", turnDisplayOff); // Turns off display completly until reboot
  server.on("/reqMiscSet", handleRequestMiscSet);
  server.on("/reqMisc", handleRequestMisc); // Misc Infos für WebConfig
  server.on("/setMisc", handleSetMisc);     // Misc ändern

  // FSBrowser initialisieren
  server.on("/list", HTTP_GET, handleFileList); // list directory
  server.on("/edit", HTTP_GET, []() {           // load editor
    if (!handleFileRead("/edit.htm"))
    {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  server.on("/edit", HTTP_PUT, handleFileCreate);    // create file
  server.on("/edit", HTTP_DELETE, handleFileDelete); // delete file
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  },
            handleFileUpload);

  server.onNotFound(handleWebRequests); // Sonstiges

  httpUpdate.setup(&server); // ESP8266HTTPUpdateServer.cpp https://github.com/esp8266/Arduino/pull/3732/files
  server.begin();
  if (startMDNS)
  {
    mdns.addService("http", "tcp", 80);
  }
}

void setupOTA()
{
  ArduinoOTA.setHostname(mqtt_clientid);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
    {
      type = "sketch";
      DBG_PRINTLN("OTA starting - updateing sketch");
    }
    else
    { // U_SPIFFS
      type = "filesystem";
      DBG_PRINTLN("OTA starting - updateing SPIFFS");
      //SPIFFS.end();
    }
    showDispOTA(0, 100);
  });
  ArduinoOTA.onEnd([]() {
    DBG_PRINTLN("OTA update finished!");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    showDispOTA(progress, total);
    DBG_PRINT("OTA in progress: ");
    DBG_PRINTLN((progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    showDispOTAEr(String(error));
    DBG_PRINT("Error: ");
    DBG_PRINTLN(error);
    if (error == OTA_AUTH_ERROR)
      DBG_PRINTLN("Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
      DBG_PRINTLN("Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
      DBG_PRINTLN("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
      DBG_PRINTLN("Receive Failed");
    else if (error == OTA_END_ERROR)
      DBG_PRINTLN("End Failed");
  });
  ArduinoOTA.begin();
}
