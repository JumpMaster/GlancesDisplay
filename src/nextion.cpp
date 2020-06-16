#include "nextion.h"
#include "NextionDownloadRK.h"
#include "nextionSecrets.h"

Nextion::Nextion(USARTSerial &serial) : serial(serial) {
}

void Nextion::resetVariables() {
  nextionVerified = false;
  nextionReady = false;
  // nextionStartup = false;
  currentPage = 0;
}

void Nextion::doUpdate() {
  upgradeState = UploadInProgress;
  firmwareUploadCompletedAt = 0;
  firmwareUploadAttempt++;
  resetVariables();
  displayDownload.withHostname(tftUploadServername).withPort(tftUploadPort).withPathPartOfUrl(tftUploadFilepath).withForceDownload();
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
  serial.begin(115200);
}

void Nextion::loop() {

  if (upgradeState == UploadInProgress) {
	  displayDownload.loop();

    if (displayDownload.getIsDone()) {
      Log.info("Upload finished");
      upgradeState = UploadComplete;
      firmwareUploadCompletedAt = Time.now();
    }
  }

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
          doUpdate();
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
      Log.info("Firmware update completed successfully");
      // Need to do something here to write the firmware update as successful.
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

void Nextion::checkReturnCode(char const *data, int length) {
  if (length == 1) {
    if (data[0] == 0x88) { // Nextion Ready
      nextionReady = true;
      currentPage = 0;
      Log.info("Ready");
      return;
    } else if (data[0] == 0x1a) {  // Invalid component
      Log.info("Invalid component");
      return;
    }
  } else if (length == 3) {
    if (data[0]+data[1]+data[2] == 0) { // Nextion Startup
      // nextionStartup = true;
      Log.info("Startup");
      return;
    }
  } else {
    if (data[0] == 0x70) { // String data
      if (!nextionVerified && strstr(data, "glances") != NULL) {
        Log.info("Firmware verified");
        nextionVerified = true;
        return;
      }
    }
  }
  Log.info("Return data length - %d. char[0] - 0x%x", length, data[0]);
}

void Nextion::execute(char const *command) {
  if (getIsReady()) {
    serial.print(command);
    serial.print("\xFF\xFF\xFF");
  }
}

void Nextion::run(char const *command) {
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

void Nextion::setPic(const int page, char const *name, const int pic) {
  char buffer[100];
  sprintf(buffer, "page%d.%s.pic=%d", page, name, pic);
  execute(buffer);
}

void Nextion::setTextPercent(const uint8_t page, const uint8_t server, char const *label, const uint8_t value) {
  char buffer[100];
  sprintf(buffer, "page%d.s%dtxt%s.txt=\"%d%%\"", page, server, label, value);
  execute(buffer);
}

void Nextion::setUptimeText(const uint8_t page, const uint8_t server, int uptime) {
  char buffer[100];
  int days = uptime / (24 * 3600);
  uptime = uptime % (24 * 3600);
  int hours = uptime / 3600;
  uptime %= 3600;
  int minutes = uptime / 60;
  if (days > 0) {
      sprintf(buffer, "page%d.s%dtxtuptime.txt=\"Uptime %dd, %02d:%02d\"", page, server, days, hours, minutes);
  } else {
      sprintf(buffer, "page%d.s%dtxtuptime.txt=\"Uptime %02d:%02d\"", page, server, hours, minutes);
  }
  
  execute(buffer);
}

void Nextion::setText(const uint8_t page, const int server, char const *name, int value) {
  setText(page, server, name, value, "");
}

void Nextion::setText(const uint8_t page, const int server, char const *name, int value, char *suffix) {
  char buffer[100];
  sprintf(buffer, "%d", value);
  setText(page, server, name, buffer, suffix);
}

void Nextion::setText(const uint8_t page, const int server, char const *name, char const *value) {
  setText(page, server, name, value, "");
}

void Nextion::setText(const uint8_t page, const int server, char const *name, char const *value, char *suffix) {
  char buffer[100];
  sprintf(buffer, "page%d.s%dtxt%s.txt=\"%s%s\"", page, server, name, value, suffix);
  execute(buffer);
}

void Nextion::setProgressBar(const uint8_t page, const uint8_t server, const uint32_t bar, uint8_t value) {
  char buffer[100];
  value = value == 0 ? 1 : value;
  if (bar == 1) {
    sprintf(buffer, "page%d.s%dpbcpu.val=%d", page, server, value);
  } else if (bar == 2) {
    sprintf(buffer, "page%d.s%dpbmemory.val=%d", page, server, value);
  } else if (bar == 3) {
    sprintf(buffer, "page%d.s%dpbswap.val=%d", page, server, value);
  }
  execute(buffer);
}

void Nextion::refreshComponent(char const *name) {
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