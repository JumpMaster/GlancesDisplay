#include "nextion.h"
#include "NextionDownloadRK.h"
#include "nextionSecrets.h"

Nextion::Nextion(USARTSerial &serial) : serial(serial) {
}

void Nextion::doUpdate() {
  displayDownload.withHostname(tftUploadServername).withPort(tftUploadPort).withPathPartOfUrl(tftUploadFilepath).withForceDownload();
	displayDownload.setup();
  firmwareUpdateInProgress = true;
}

void Nextion::setPower(bool on) {
  digitalWrite(A0, on);
}

void Nextion::setup() {
  pinMode(A0, OUTPUT);
  serial.begin(115200);
}

void Nextion::loop() {
  if (firmwareUpdateInProgress) {
	  displayDownload.loop();

    if (displayDownload.getHasRun() && displayDownload.getIsDone()) {
      Log.info("Upload finished");
      nextionReady = false;
      nextionStartup = false;
      firmwareUpdateInProgress = false;
    }
  }

  while (!firmwareUpdateInProgress && Serial1.available()) {
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
      return;
    } else if (data[0] == 0x1a) {  // Invalid component
      Log.info("Invalid component");
      return;
    }
  } else if (length == 3) {
    if (data[0]+data[1]+data[2] == 0) { // Nextion Startup
      nextionStartup = true;
      return;
    }
  }
  Log.info("Length - %d", length);
  Log.info("Return data char[0] - 0x%x", data[0]);
}

void Nextion::execute(char const *command) {
  if (!nextionReady || firmwareUpdateInProgress) {
    return;
  }
  serial.print(command);
  serial.print("\xFF\xFF\xFF");
}

void Nextion::run(char const *command) {
  // serial.write(command);
  execute(command);
}

void Nextion::reset() {
    execute("rest");
}

void Nextion::setBaud(int speed) {
  char buffer[100];
  sprintf(buffer, "baud=%d", speed);
  // serial.print(buffer);
  execute(buffer);
}

void Nextion::setPic(const int page, char const *name, const int pic) {
  char buffer[100];
  sprintf(buffer, "page%d.%s.pic=%d", page, name, pic);
  // serial.print(buffer);
  execute(buffer);
}

void Nextion::setTextPercent(const uint8_t page, const uint8_t server, char const *label, const uint8_t value) {
  char buffer[100];
  sprintf(buffer, "page%d.s%dtxt%s.txt=\"%d%%\"", page, server, label, value);
  // serial.print(buffer);
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
  
  // serial.print(buffer);
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
  // serial.print(buffer);
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
  // serial.print(buffer);
  execute(buffer);
}

void Nextion::refreshComponent(char const *name) {
  // String cmd = "ref ";
  char buffer[100];
  sprintf(buffer, "ref %s", name);
  // cmd += name;
  // serial.print(cmd);
  execute(buffer);
}

void Nextion::setPage(const uint8_t page) {
  // serial.print("page "+String(page, DEC));
  currentPage = page;
  char buffer[8];
  sprintf(buffer, "page %d", currentPage);
  execute(buffer);
}

void Nextion::setBrightness(const uint8_t brightness) {
  // serial.print("dim="+String(brightness, DEC));
  char buffer[8];
  sprintf(buffer, "dim=%d", brightness);
  execute(buffer);
}

void Nextion::setSleep(const bool sleep) {
  // serial.print("sleep="+String(sleep, DEC));
  char buffer[8];
  sprintf(buffer, "sleep=%d", sleep);
  execute(buffer);
}

void Nextion::stopRefreshing() {
  // serial.print("ref_stop");
  execute("ref_stop");
}

void Nextion::startRefreshing() {
  // serial.print("ref_star");
  execute("ref_star");
}