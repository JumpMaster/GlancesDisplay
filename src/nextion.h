#ifndef Nextion_h
#define Nextion_h

#include "application.h"

#define nexSerial Serial1

class Nextion
{
public:
  Nextion(void);

  void execute();
  void run(String command);
  void setBaud(int speed);
  void setPic(const int page, const char* name, const int pic);
  void setText(const int server, const char* name, int value);
  void setText(const int server, const char* name, int value, char* suffix);
  void setText(const int server, const char* name, const char* value);
  void setText(const int server, const char* name, const char* value, char* suffix);
  void setTextPercent(const uint8_t server, const char* label, const uint8_t value);
  void setUptimeText(const uint8_t server, int uptime);
  void setLoadText(const uint8_t server, uint8_t load, const char* load_value);
  void refreshComponent(const char* name);
  void setPage(const int page);
  void setBrightness(const int brightness);
  void setSleep(const bool sleep);
  void setProgressBar(const uint8_t server, const uint32_t bar, const uint8_t value);
  void stopRefreshing();
  void startRefreshing();
  void reset();

protected:
};

#endif