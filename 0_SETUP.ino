void setup() {
    Serial.begin(115200);
  
  // Add Listeners
  lastToggledSys = millis();
  lastToggledSen = millis();
  lastToggledAct = millis();
  lastToggledInd = millis();

  gEM.addListener( EventManager::cbpiEventSystem, listenerSystem );
  gEM.addListener( EventManager::cbpiEventSensors, listenerSensors );
  gEM.addListener( EventManager::cbpiEventActors, listenerActors );
  gEM.addListener( EventManager::cbpiEventInduction, listenerInduction );

  // Sensoren Starten
  DS18B20.begin();

  // Dateisystem laden
  ESP.wdtFeed();
  if (!SPIFFS.begin())  {
  DBG_PRINT("SPIFFS Mount failed");
  }
  Dir dir = SPIFFS.openDir("/");
  Serial.printf("\n");
  while (dir.next()) {
    String fileName = dir.fileName();
    size_t fileSize = dir.fileSize();
    Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
  }
 
  // Set device name
  snprintf(mqtt_clientid, 25, "ESP8266-%08X", mqtt_chip_key);
  
  // Einstellungen laden
  ESP.wdtFeed();
  loadConfig();
  showDispCbpi();
  delay(2000);
  
  // WiFi Manager
  ESP.wdtFeed();
  WiFi.hostname(mqtt_clientid);
  WiFiManagerParameter cstm_mqtthost("host", "cbpi ip", mqtthost, 16);
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  wifiManager.addParameter(&cstm_mqtthost);
  //wifiManager.autoConnect("MQTTDevice");
  wifiManager.autoConnect(mqtt_clientid);
  strcpy(mqtthost, cstm_mqtthost.getValue());

  // mDNS laden
  ESP.wdtFeed();
  if (!MDNS.begin(mqtt_clientid)) {
    Serial.println("Error setting up MDNS responder!");
  }

  // MQTT starten
  client.setServer(mqtthost, 1883);
  client.setCallback(mqttcallback);

  // Änderungen speichern
  ESP.wdtFeed();
  saveConfig();

  // ArduinoOTA aktivieren
  setupOTA();

  // FSBrowser initialisieren
  // list directory
  server.on("/list", HTTP_GET, handleFileList);
  // load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) {
      server.send(404, "text/plain", "FileNotFound");
    }
  });
  // create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  // delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  // first callback is called after the request has ended with all parsed arguments
  // second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  // get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });

  // Webserver starten
  ESP.wdtFeed();
  setupServer();
}

void setupServer() {
  
  server.on("/", handleRoot);

  server.on("/setupActor", handleSetActor);       // Einstellen der Aktoren
  server.on("/setupSensor", handleSetSensor);     // Einstellen der Sensoren

  server.on("/reqSensors", handleRequestSensors); // Liste der Sensoren ausgeben
  server.on("/reqActors", handleRequestActors);   // Liste der Aktoren ausgeben
  server.on("/reqInduction", handleRequestInduction);

  server.on("/reqSearchSensorAdresses", handleRequestSensorAddresses);
  server.on("/reqPins", handlereqPins);

  server.on("/reqSensor", handleRequestSensor);   // Infos der Sensoren für WebConfig
  server.on("/reqActor", handleRequestActor);     // Infos der Aktoren für WebConfig
  server.on("/reqIndu", handleRequestIndu);       // Infos der Indu für WebConfig

  server.on("/setSensor", handleSetSensor);       // Sensor ändern
  server.on("/setActor", handleSetActor);         // Aktor ändern
  server.on("/setIndu", handleSetIndu);           // Indu ändern
  
  server.on("/delSensor", handleDelSensor);       // Sensor löschen
  server.on("/delActor", handleDelActor);         // Aktor löschen

  server.on("/reboot", rebootDevice);             // reboots the whole Device
  server.on("/mqttOff", turnMqttOff);             // Turns off MQTT completly until reboot

#ifdef DISPLAY
  server.on("/reqDisplay", handleRequestDisplay);
  server.on("/reqDisp", handleRequestDisp);       // Infos Display für WebConfig
  server.on("/setDisp", handleSetDisp);           // Display ändern
  server.on("/displayOff", turnDisplayOff);       // Turns off display completly until reboot
#endif

  server.onNotFound(handleWebRequests);           // Sonstiges

  server.begin();
}

void setupOTA() {
  ArduinoOTA.begin();
  ArduinoOTA.onStart([]() {
    DBG_PRINTLN("OTA starting...");
    showDispSet("OTA started");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA in progress: %u%%\r\n", (progress / (total / 100)));
  });
  ArduinoOTA.onEnd([]() {
    DBG_PRINTLN("OTA update finished!");
    showDispSet("OTA finished");
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    showDispSet("!!!OTA Error !!!");
  });
}
