#include "nextion.h"
#include "NextionDownloadRK.h"
#include "nextionSecrets.h"

Nextion::Nextion(USARTSerial &serial) : serial(serial), displayDownload(serial, 0) {}

void Nextion::resetVariables() {
  nextionVerified = false;
  nextionReady = false;
  currentPage = 0;
}

void Nextion::doUpdate(bool force) {
  if (upgradeState != Idle) {
    Log.info("upgradeState != Idle");
    return;
  }
  upgradeState = UploadInProgress;
  firmwareUploadCompletedAt = 0;
  firmwareUploadAttempt++;
  resetVariables();
  displayDownload.withHostname(tftUploadServername).withPort(tftUploadPort).withPathPartOfUrl(tftUploadFilepath);
  if (force) {
    displayDownload.withForceDownload();
  }
	displayDownload.setup();
}

void Nextion::setPower(bool on) {
  digitalWrite(A0, on);
  powerState = on;
  powerStateChangedAt = Time.now();

  if (!on) {
    resetVariables();
  }
}

void Nextion::setup() {
  pinMode(A0, OUTPUT);
  digitalWrite(A0, LOW);
  serial.begin(921600);
}

void Nextion::loop() {

  if (upgradeState == UploadInProgress || upgradeState == CheckInProgress) {
	  displayDownload.loop();

    if (displayDownload.getIsDone()) {
      if (upgradeState == UploadInProgress) {
        Log.info("Upload finished");
        upgradeState = UploadComplete;
        firmwareUploadCompletedAt = Time.now();
      } else if (upgradeState == CheckInProgress) {
        upgradeState = Idle;
        if (displayDownload.upgradeAvailable()) {
          Log.info("Upgrade available");
          doUpdate(false);
        }
      }
    }
  }
/*
  if (Time.now() - lastFirmwareUpdateCheck > 300) {
    lastFirmwareUpdateCheck = Time.now();
    upgradeState = CheckInProgress;
    displayDownload.withHostname(tftUploadServername).withPort(tftUploadPort).withPathPartOfUrl(tftUploadFilepath).withUpgradeAvailableCheckOnly();
	  displayDownload.setup();
  }
*/
  // Upload has completed.
  if (upgradeState >= UploadComplete && firmwareUploadCompletedAt > 0) {
    if (!nextionVerified && Time.now() - firmwareUploadCompletedAt >= 10) {
      if (upgradeState == UploadComplete) {
        if (firmwareUploadAttempt >= 3) {
          Log.info("Firmware upload failed after %d attempts", firmwareUploadAttempt);
          firmwareUploadCompletedAt = 0;
          firmwareUploadAttempt = 0;
          upgradeState = Idle;
        } else {
          Log.info("10+ seconds since firmware upload completed, something went wrong.");
          upgradeState = UploadFailed_PowerOff;
          powerOff();
        }
      } else if (upgradeState == UploadFailed_PowerOff && Time.now() - powerStateChangedAt > 5) {
        powerOn();
        upgradeState = UploadFailed_PowerOn;
      } else if (upgradeState == UploadFailed_PowerOn && Time.now() - powerStateChangedAt > 5) {
        if (nextionReady) {
          Log.info("Starting firmware upload attempt %d", firmwareUploadAttempt+1);
          doUpdate(false);
        } else {
          Log.info("Nextion is not ready after 5 seconds.  Aborting.");
          firmwareUploadCompletedAt = 0;
          firmwareUploadAttempt = 0;
          upgradeState = Idle;
        }
      }
    } else if (nextionVerified) {
      firmwareUploadCompletedAt = 0;
      firmwareUploadAttempt = 0;
      upgradeState = Idle;
      displayDownload.saveFirmwareVersion();
      Log.info("Firmware update completed successfully");
    }
  }

  while (upgradeState != UploadInProgress && Serial1.available()) {
    int byte = Serial1.read();

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
}

void Nextion::checkReturnCode(const char* data, int length) {
  switch (data[0]) {
    case 0: // Startup
      if (data[0]+data[1]+data[2] == 0) { // Nextion Startup
        // nextionStartup = true;
        Log.info("Startup");
        return;
      }
      break;
    case 0x63: // Connect
      Log.info(data);
      nextionConnected = true;
      getVersion();
      break;
    case 0x70: // String data
      if (!nextionVerified && strstr(data, "glances") != NULL) {
        Log.info("Firmware verified");
        nextionVerified = true;
      }
      Log.info(data);
      break;
    case 0x88: // Ready
      nextionReady = true;
      currentPage = 0;
      Log.info("Ready");
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

void Nextion::execute(const char* command) {
  if (upgradeState <= CheckInProgress) {
    serial.print(command);
    serial.print("\xFF\xFF\xFF");
  }
}

void Nextion::run(const char* command) {
  execute(command);
}

void Nextion::reset() {
    execute("rest");
}

void Nextion::setBaud(int speed) {
  char buffer[100];
  sprintf(buffer, "baud=%d", speed);
  execute(buffer);
}

void Nextion::setPic(const int page, const char* name, const int pic) {
  char buffer[100];
  sprintf(buffer, "page%d.%s.pic=%d", page, name, pic);
  execute(buffer);
}

void Nextion::setText(const char* name, const char* value) {
  char buffer[100];
  sprintf(buffer, "%s.txt=\"%s\"", name, value);
  execute(buffer);
}

void Nextion::setProgressBar(const char* name, const uint8_t value) {
  char buffer[100];
  sprintf(buffer, "%s.val=%d", name, value == 0 ? 1 : value);
  execute(buffer);
}

void Nextion::setForegroundColor(const char* name, uint16_t color) {
  char buffer[100];
  sprintf(buffer, "%s.pco=%d", name, color);
  execute(buffer);
}

void Nextion::refreshComponent(const char* name) {
  // String cmd = "ref ";
  char buffer[100];
  sprintf(buffer, "ref %s", name);
  execute(buffer);
}

void Nextion::setPage(const uint8_t page) {
  currentPage = page;
  char buffer[8];
  sprintf(buffer, "page %d", currentPage);
  execute(buffer);
}

void Nextion::setBrightness(const uint8_t brightness) {
  char buffer[8];
  sprintf(buffer, "dim=%d", brightness);
  execute(buffer);
}

void Nextion::setSleep(const bool sleep) {
  char buffer[8];
  sprintf(buffer, "sleep=%d", sleep);
  execute(buffer);
}

void Nextion::stopRefreshing() {
  execute("ref_stop");
}

void Nextion::startRefreshing() {
  execute("ref_star");
}