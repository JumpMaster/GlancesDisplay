#include "TJC.h"
#include "TJCDownload.h"

TJC::TJC(USARTSerial &serial, int baud) : serial(serial), tjcDownload(serial, baud) {
    baudRate = baud;
}

void TJC::setup() {
    serial.begin(baudRate);
    pinMode(A0, OUTPUT);
    digitalWrite(A0, LOW);
}

void TJC::resetVariables() {
    tjcStartup = false;
    tjcReady = false;
    tjcConnected = false;
    tjcVerified = false;
    currentPage = 0;
}


void TJC::getVersion() {
    deviceVersionRequestedAt = millis();
    execute("get global.version.txt");
}


void TJC::doUpdate(bool firmware) {//bool force) {
    // if (upgradeState != Idle) {
        // Log.info("upgradeState != Idle");
        // return;
    // }
    // upgradeState = UploadInProgress;
    // firmwareUploadCompletedAt = 0;
    // firmwareUploadAttempt++;
    
    if (!firmware) {
        tjcDownload.start(TJCDownload::JSON_SCHEMA);
    } else {
        tjcDownload.start(TJCDownload::TFT_FIRMWARE);
        resetVariables();
    }
//   displayDownload.withHostname(tftUploadServername).withPort(tftUploadPort).withPathPartOfUrl(tftUploadFilepath);
//   if (force) {
  // displaDownload.withForceDownload();
//   }
  // displayDownload.setup();
  // TJCDownload.start(TJCDownload::JSON_SCHEMA);
}


void TJC::setPower(bool on) {
    digitalWrite(A0, on);
    powerState = on;
    powerStateChangedAt = Time.now();

    if (!on) {
        resetVariables();
    }
}

void TJC::loop() {

    //
    // Check for new firmware
    //
    if (tjcVerified && upgradeState == Idle && millis() > nextFirmwareUpdateCheck) {
        tjcDownload.start(tjcDownload.JSON_SCHEMA);
        upgradeState = VersionCheckInProgress;
        nextFirmwareUpdateCheck = millis() + firmwareUpdateCheckInterval;
    }

    if (deviceVersionRequestedAt > 0 && millis() > deviceVersionRequestedAt + 1000) {
        deviceVersionRequestedAt = 0;
        Log.info("Timeout: Did not receive a valid device version");
    }

    //
    //  Something something do firmware upgrade
    //

    //  if there's an upgrade available.
    //  Reset variables
    //  Save the available version number
    //  Do the update
    if (automaticUpgradesEnabled && tjcDownload.getIsUpdateAvailable() && firmwareUploadAttempt == 0) {
        firmwareUploadAttempt++;
        upgradeState = UploadInProgress;
        resetVariables();
        strcpy(deviceVersionNumberUploaded, tjcDownload.getAvailableVersion());
        tjcDownload.start(tjcDownload.TFT_FIRMWARE);
    }

    //
    // Runs checks once upgrades are compete
    //
    if (upgradeState != Idle && tjcDownload.getIsDone()) {
        if (upgradeState == VersionCheckInProgress) {
            tjcFirmwareChecked = true;
            upgradeState = Idle;
        } else if (upgradeState == UploadInProgress) {
            Log.info("Firmware upload complete");
            upgradeState = UploadComplete;
            firmwareUploadCompletedAt = millis();
        } else if (upgradeState == UploadComplete) {
            if (tjcVerified == true) { // TODO: Work on a better upgrade successful check and handle errors
                // Need to check upgraded version is as expected here.
                Log.info("Upgrade confirmed successful");
                upgradeState = Idle;
                firmwareUploadAttempt = 0;
                firmwareUploadCompletedAt = 0;
            } else if (millis() > firmwareUploadCompletedAt + 5000) {
                Log.info("Firmware upgrade probably failed, I can't handle this yet.");
                automaticUpgradesEnabled = false;
                upgradeState = Idle;
                firmwareUploadAttempt = 0;
                firmwareUploadCompletedAt = 0;
            }
        }
    }

    //
    // Read serial data if we're not busy
    //
	while (upgradeState != UploadInProgress && serial.available()) {
		int byte = serial.read();
		// Log.info("%x", byte);
	 	serialData[serialPosition++] = byte;
	 	if (byte == 0xff && serialPosition >= 3) {
			if (serialData[serialPosition-1] == 0xff &&
			 		serialData[serialPosition-2] == 0xff &&
			 		serialData[serialPosition-3] == 0xff) {
		  		serialData[serialPosition-3] = '\0';
		  		checkReturnCode(serialData, serialPosition-3);
		  		serialPosition = 0;
		  	}
	 	}
	}

    tjcDownload.loop();
}


void TJC::checkReturnCode(const char* data, int length) {
    switch (data[0]) {
        case 0: // Startup
           if (length == 3 && data[0]+data[1]+data[2] == 0) { // TJC Startup
                tjcStartup = true;
		        Log.info("Startup");
		        return;
		    }
		    break;
	    case 0x63: // Connect
		    // Log.info(data);
            {
                char d[length];
                strcpy(d, data);
                char *p = strtok(d, ",");
                for (int i = 0; i < 7; i++) {
                    if (p == NULL)
                        break;
                    
                    switch (i) {
                        case 0:
                            // Log.info("comok - %s\n", p);
                            break;
                        case 1:
                            // Log.info("reserved - %s\n", p);
                            break;
                        case 2:
                            // Log.info("model - %s\n", p);
                            break;
                        case 3:
                            // Log.info("firmware - %s\n", p);
                            break;
                        case 4:
                            // Log.info("mcu - %s\n", p);
                            break;
                        case 5:
                            if (strlen(p) == 16) {
                                Log.info("Device Serial=\"%s\"", p);
                                tjcDownload.setSerialNumber(p);
                            } else {
                                Log.info("serial - invalid length");
                            }
                            break;
                        case 6:
                            // Log.info("flash size - %s\n", p);
                            break;
                    }
                    p = strtok(NULL, ",");
                }
            }
            tjcConnected = true;
            Log.info("Connected");
            getVersion();
            break;
        case 0x66: // sendme page number
            currentPage = data[1];
            if (pageChangeCbPtr != NULL) {
                pageChangeCbPtr(currentPage);
            }
            break;
        case 0x70: // String data
            char buffer[100];
            memcpy(buffer, &data[1], strlen(data)-1);
            // Log.info("\"%s\"", buffer);
            
            if (deviceVersionRequestedAt > 0) {
                deviceVersionRequestedAt = 0;

                const size_t capacity = 300;//JSON_OBJECT_SIZE(2) + 30;
                DynamicJsonDocument doc(capacity);

                // Parse JSON object
                DeserializationError error = deserializeJson(doc, buffer);

                if (!error) {
                    if (strlen(doc["version"]) < 12) {
                        Log.info("Device Version=\"%s\"", doc["version"].as<const char*>());
                        tjcDownload.setVersionNumber(doc["version"]);
                        tjcVerified = true;
                        Log.info("Verified");
                    }
                } else {
                    Log.info("It's null");
                }
            } else if (stringDataCbPtr != NULL) {
                stringDataCbPtr(buffer);
            }
		    break;
        case 0x71: // Numeric data
            if (numericDataCbPtr != NULL) {
                uint32_t n = data[1] + data[2]*256 + data[3]*65535 + data[4]*16777216;
                numericDataCbPtr(n);
            }
            break;
        case 0x88: // Ready
            tjcReady = true;
            currentPage = 0;
            Log.info("Ready");
            delay(500); // TODO: Add delay system
            sendConnect();
            break;
        case 0x1a: // Invalid component
            Log.info("Invalid component");
            break;
        default:
            Log.info("Return data length - %d. char[0] - 0x%x", length, data[0]);
            break;
    }
}

void TJC::execute(const char* command) {
  // if (upgradeState <= CheckInProgress) {
	 serial.print(command);
	 serial.print("\xFF\xFF\xFF");
  // }
}


void TJC::setText(const char* name, const char* value) {
  char buffer[100];
  sprintf(buffer, "%s.txt=\"%s\"", name, value);
  execute(buffer);
}


void TJC::setProgressBar(const char* name, const uint8_t value) {
  char buffer[100];
  sprintf(buffer, "%s.val=%d", name, value == 0 ? 1 : value);
  execute(buffer);
}


void TJC::setForegroundColor(const char* name, uint16_t color) {
  char buffer[100];
  sprintf(buffer, "%s.pco=%d", name, color);
  execute(buffer);
}


void TJC::setPage(const uint8_t page) {
  currentPage = page;
  char buffer[8];
  sprintf(buffer, "page %d", currentPage);
  execute(buffer);
}

void TJC::attachPageChangeCallback(PageChangeEventCb cb) {
    pageChangeCbPtr = cb;
}

void TJC::attachStringDataCallback(StringDataCb cb) {
    stringDataCbPtr = cb;
}

void TJC::attachNumericDataCallback(NumericDataCb cb) {
    numericDataCbPtr = cb;
}