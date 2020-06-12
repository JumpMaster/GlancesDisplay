#include "nextion.h"

Nextion::Nextion()
{
}

void Nextion::execute() {
  nexSerial.print("\xFF\xFF\xFF");
//   nexSerial.write(0xFF);
//   nexSerial.write(0xFF);
//   nexSerial.write(0xFF);
}

void Nextion::run(String command) {
  nexSerial.write(command);
  execute();
}

void Nextion::reset() {
    nexSerial.print("rest");
    execute();
}

void Nextion::setBaud(int speed) {
  char buffer[100];
  sprintf(buffer, "baud=%d", speed);
  nexSerial.print(buffer);
  execute();
}

void Nextion::setPic(const int page, const char* name, const int pic) {
  char buffer[100];
  sprintf(buffer, "page%d.%s.pic=%d", page, name, pic);
  nexSerial.print(buffer);
  execute();
}

void Nextion::setTextPercent(const uint8_t server, const char* label, const uint8_t value) {
  char buffer[100];
  sprintf(buffer, "s%dtxt%s.txt=\"%d%%\"", server, label, value);
  nexSerial.print(buffer);
  execute();
}

void Nextion::setUptimeText(const uint8_t server, int uptime) {
    char buffer[100];
    int days = uptime / (24 * 3600); 
    uptime = uptime % (24 * 3600); 
    int hours = uptime / 3600; 
    uptime %= 3600; 
    int minutes = uptime / 60;
    if (days > 0) {
        sprintf(buffer, "s%dtxtuptime.txt=\"%dd, %02d:%02d\"", server, days, hours, minutes);
    } else {
        sprintf(buffer, "s%dtxtuptime.txt=\"%02d:%02d\"", server, hours, minutes);
    }
  
  nexSerial.print(buffer);
  execute();
}

void Nextion::setLoadText(const uint8_t server, uint8_t load, const char* load_value) {
  char buffer[100];
  sprintf(buffer, "s%dtxtload%d.txt=\"%s\"", server, load, load_value);
  nexSerial.print(buffer);
  execute();
}

void Nextion::setText(const int server, const char* name, int value) {
  setText(server, name, value, "");
}

void Nextion::setText(const int server, const char* name, int value, char* suffix) {
  char buffer[100];
  sprintf(buffer, "%d", value);
  setText(server, name, buffer, suffix);
}

void Nextion::setText(const int server, const char* name, const char* value) {
  setText(server, name, value, "");
}

void Nextion::setText(const int server, const char* name, const char* value, char* suffix) {
  char buffer[100];
  sprintf(buffer, "s%dtxt%s.txt=\"%s%s\"", server, name, value, suffix);
  nexSerial.print(buffer);
  execute();
}

void Nextion::setProgressBar(const uint8_t server, const uint32_t bar, uint8_t value) {
  char buffer[100];
  value = value == 0 ? 1 : value;
  if (bar == 1) {
    sprintf(buffer, "s%dpbcpu.val=%d", server, value);
  } else if (bar == 2) {
    sprintf(buffer, "s%dpbmemory.val=%d", server, value);
  } else if (bar == 3) {
    sprintf(buffer, "s%dpbswap.val=%d", server, value);
  }
  nexSerial.print(buffer);
  execute();
}

void Nextion::refreshComponent(const char* name) {
  String cmd = "ref ";
  cmd += name;
  nexSerial.print(cmd);
  execute();
}

void Nextion::setPage(const int page) {
  nexSerial.print("page "+String(page, DEC));
  execute();
}

void Nextion::setBrightness(const int brightness) {
  nexSerial.print("dim="+String(brightness, DEC));
  execute();
}

void Nextion::setSleep(const bool sleep) {
  nexSerial.print("sleep="+String(sleep, DEC));
  execute();
}

void Nextion::stopRefreshing() {
  nexSerial.print("ref_stop");
  execute();
}

void Nextion::startRefreshing() {
  nexSerial.print("ref_star");
  execute();
}