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
  void run(const char* command);
  void setBaud(int speed);
  bool getIsReady() { return nextionReady && nextionVerified && upgradeState < UploadInProgress; }
  void doUpdate(bool force);
  void setPic(const int page, const char* name, const int pic);
  void setText(const char* name, const char* value);
  void refreshComponent(const char* name);
  uint8_t getPage() { return currentPage; }
  void setPage(const uint8_t page);
  void setBrightness(const uint8_t brightness);
  void setSleep(const bool sleep);
  void setProgressBar(const char* name, const uint8_t value);
  void setForegroundColor(const char* name, uint16_t color);

  void stopRefreshing();
  void startRefreshing();
  void reset();
  void powerOn() { setPower(true); }
  void powerOff() { setPower(false); }

protected:
  typedef enum {
    Idle = 0,
    CheckInProgress = 1,
    UploadInProgress = 2,
    UploadComplete = 3,
    UploadFailed_PowerOff = 4,
    UploadFailed_PowerOn = 5,
  } UpgradeStatus;

  void resetVariables();
  void setPower(bool on);
  void checkReturnCode(const char* data, int length);
  char serialData[100];
  uint8_t serialPosition = 0;
  USARTSerial &serial;
  NextionDownload displayDownload;
  void execute(const char* command);
  uint8_t currentPage = 0;

  bool nextionReady = false;
  bool nextionVerified = false;

  bool powerState = false;
  uint32_t powerStateChangedAt = 0;

  UpgradeStatus upgradeState = Idle;
  uint32_t firmwareUploadCompletedAt = 0;
  uint32_t lastFirmwareUpdateCheck = 0;
  uint8_t firmwareUploadAttempt = 0;
};

#endif