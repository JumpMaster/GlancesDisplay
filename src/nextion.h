#ifndef Nextion_h
#define Nextion_h

#include "Particle.h"
#include "NextionDownloadRK.h"

class Nextion
{
public:
  Nextion(USARTSerial &serial);

  void setup();
  void loop();
  void run(char const *command);
  void setBaud(int speed);
  bool getIsReady() { return nextionReady && nextionVerified && !firmwareUpdateInProgress;}
  void doUpdate();
  void setPic(const int page, char const *name, const int pic);
  void setText(const uint8_t page, const int server, char const *name, int value);
  void setText(const uint8_t page, const int server, char const *name, int value, char *suffix);
  void setText(const uint8_t page, const int server, char const *name, char const *value);
  void setText(const uint8_t page, const int server, char const *name, char const *value, char *suffix);
  void setTextPercent(const uint8_t page, const uint8_t server, char const *label, const uint8_t value);
  void setUptimeText(const uint8_t page, const uint8_t server, int uptime);
  void setLoadText(const uint8_t page, const uint8_t server, uint8_t load, char const *load_value);
  void refreshComponent(char const *name);
  uint8_t getPage() { return currentPage; }
  void setPage(const uint8_t page);
  void setBrightness(const uint8_t brightness);
  void setSleep(const bool sleep);
  void setProgressBar(const uint8_t page, const uint8_t server, const uint32_t bar, const uint8_t value);
  void stopRefreshing();
  void startRefreshing();
  void reset();
  void powerOn() { setPower(true); }
  void powerOff() { setPower(false); }

protected:
  void resetVariables();
  void setPower(bool on);
  void checkReturnCode(char const *data, int length);
  char serialData[100];
  uint8_t serialPosition = 0;
  NextionDownload displayDownload;//Serial1, 0);
  void execute(char const *command);
  USARTSerial &serial;
  
  bool nextionReady = false;
  bool nextionVerified = false;
  // bool nextionStartup = false;

  bool powerState = false;
  uint32_t powerStateChangedAt = 0;

  uint8_t currentPage = 0;

  bool firmwareUpdateInProgress = false;
  bool firmwareUpdateSuccessful = false;
  uint32_t firmwareUploadCompletedAt = 0;
};

#endif