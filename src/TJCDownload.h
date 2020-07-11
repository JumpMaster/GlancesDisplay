#ifndef __TJCDOWNLOAD_H
#define __TJCDOWNLOAD_H

#include "Particle.h"
#include "ArduinoJson-v6.15.2.h"

class TJCDownload {
public:
	typedef enum {
		JSON_SCHEMA = 0,
		TFT_FIRMWARE = 1,
	} DOWNLOAD_TYPE;
	
	TJCDownload(USARTSerial &serial, int baud);
	virtual ~TJCDownload();
	
	void start(DOWNLOAD_TYPE downloadType);
	void loop();
    void setSerialNumber(const char* serialNumber);
    void setVersionNumber(const char* serialNumber);
	bool getIsDone() { return isDone; };
    bool getIsSuccess() { return isSuccess; };
    bool getIsUpdateAvailable() { return updateAvailable; };
    const char *getAvailableVersion() { return availableFirmwareVersion; };

protected:
	static const size_t DATE_BUFFER_SIZE = 32;
	static const size_t BUFFER_SIZE = 4096; // This size is part of the tjc protocol and can't really be changed
	static const unsigned long DATA_TIMEOUT_TIME_MS = 60000;
	static const unsigned long RETRY_WAIT_TIME_MS = 30000;
	static const unsigned long restartWaitTime = 4000;
	
	// State handlers
	void startState(void);
	void waitConnectState(void);
	void headerWaitState(void);
	void dataWaitState(void);
	void retryWaitState(void);
	void restartWaitState(void);
	void cleanupState(void);
	void doneState(void);


	void requestCheck();
	bool readAndDiscard(uint32_t timeoutMs, bool exitAfter05);
	void readAvailableAndDiscard();
	void sendCommand(const char *fmt, ...);
	bool startDownload();

	// Settings
	USARTSerial &serial;
	const char *hostname = "192.168.16.10";
	const int port = 8888;
	const char *jsonPath = "/";
	const char *jsonFilename = "hmi.json";
	const char *firmwarePath = "/";
	char availableFirmwareFilename[20];  // = "glances.tft";
	char availableFirmwareVersion[12];  // = "000.000.000"
	int downloadBaud = 115200;
	char deviceSerialNumber[17];
    char deviceVersionNumber[12]; // 000.000.000
	
	// Data
	char jsonModifiedDate[DATE_BUFFER_SIZE];


	// Misc stuff
	TCPClient client;
	char *buffer = 0;
	size_t bufferOffset;
	size_t bufferSize;
	size_t dataOffset;
	size_t dataSize;
// 	bool hasRun = false;
	bool isDone = false;
    bool isSuccess = false;
    bool updateAvailable = false;
	DOWNLOAD_TYPE downloadType = JSON_SCHEMA;
	

	// State handler stuff
	std::function<void(TJCDownload&)> stateHandler;  // = &TJCDownload::startState;
	unsigned long stateTime = 0;
};

#endif /* __TJCDOWNLOAD_H */
