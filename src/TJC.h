#ifndef TJC_h
#define TJC_h

#include "Particle.h"
#include "TJCDownload.h"
#include "ArduinoJson-v6.15.2.h"

typedef void (*PageChangeEventCb)(uint8_t);
typedef void (*NumericDataCb)(int);
typedef void (*StringDataCb)(const char*);

class TJC
{
public:
    TJC(USARTSerial &serial, int baud);

    void setup();
    void loop();
    void run(const char* command);
    void sendConnect() { execute("connect"); }
    void getVersion();
    bool getIsReady() { return tjcReady && tjcConnected && tjcVerified && tjcFirmwareChecked && upgradeState == Idle; }
    void doUpdate(bool firmware);

    uint8_t getPage() { return currentPage; }
    void setPage(const uint8_t page);
    void setPic(const char* name, const int pic);
    void setText(const char* name, const char* value);
    void refreshComponent(const char* name);
    void setBrightness(const uint8_t brightness);
    void setSleep(const bool sleep);
    void setProgressBar(const char* name, const uint8_t value);
    void setForegroundColor(const char* name, uint16_t color);

    void stopRefreshing();
    void startRefreshing();
    void reset();
    void powerOn() { setPower(true); }
    void powerOff() { setPower(false); }

    void attachPageChangeCallback(PageChangeEventCb cb);
    void attachStringDataCallback(StringDataCb cb);
    void attachNumericDataCallback(NumericDataCb cb);

protected:
    typedef enum {
        Idle = 0,
        VersionCheckInProgress = 1,
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
    TJCDownload tjcDownload;
    void execute(const char* command);
    uint8_t currentPage = 0;
    int baudRate = 9600;

    bool tjcStartup = false;
    bool tjcReady = false;
    bool tjcConnected = false;
    bool tjcVerified = false;
    bool tjcFirmwareChecked = false;

    uint32_t deviceVersionRequestedAt = 0;
    bool powerState = false;
    uint32_t powerStateChangedAt = 0;

    UpgradeStatus upgradeState = Idle;
    uint32_t nextFirmwareUpdateCheck = 0;
    bool automaticUpgradesEnabled = true;

    uint32_t firmwareUploadCompletedAt = 0;
    uint8_t firmwareUploadAttempt = 0;
    uint32_t firmwareUpdateCheckInterval = 60000; // 1 minute
    char deviceVersionNumberUploaded[17];

    PageChangeEventCb pageChangeCbPtr;
    NumericDataCb numericDataCbPtr;
    StringDataCb stringDataCbPtr;

};

#endif