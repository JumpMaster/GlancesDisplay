#include "TJCDownload.h"


// https://www.itead.cc/blog/tjc-hmi-upload-protocol

TJCDownload::TJCDownload(USARTSerial &serial, int baud) : serial(serial) {
    downloadBaud = baud;
}


TJCDownload::~TJCDownload() {
}


void TJCDownload::start(DOWNLOAD_TYPE downloadType) {
    if (downloadType == JSON_SCHEMA) {
        if (strlen(deviceSerialNumber) == 16) {
		    Log.info("Starting upgrade schema check");
        } else {
            Log.info("json check requested but no serial present");
        }
	} else {
		Log.info("Starting firmware update");
	}

	this->downloadType = downloadType;
	isDone = false;
    isSuccess = false;
	// State handler stuff
	stateHandler = &TJCDownload::startState;
}


void TJCDownload::loop() {
	if (stateHandler != NULL) {
		stateHandler(*this);
	}
}


void TJCDownload::setSerialNumber(const char* serialNumber) {
    strcpy(deviceSerialNumber, serialNumber);
}


void TJCDownload::setVersionNumber(const char* versionNumber) {
    strcpy(deviceVersionNumber, versionNumber);
    if (strlen(availableFirmwareVersion)) {
        if (strcmp(deviceVersionNumber, availableFirmwareVersion) == 0) {
            updateAvailable = false;
        } else {
            updateAvailable = true;
        }
    }
}


void TJCDownload::readAvailableAndDiscard() {
	while(serial.available()) {
		(void) serial.read();
	}
}


void TJCDownload::sendCommand(const char *fmt, ...) {
	readAvailableAndDiscard();

	char buf[64];
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);

	serial.write(buf);

	serial.write(0xff);
	serial.write(0xff);
	serial.write(0xff);
}


bool TJCDownload::startDownload() {

	Log.info("start download dataSize=%d downloadBaud=%d", dataSize, downloadBaud);

	sendCommand("");
	sendCommand("whmi-wri %d,%d,0", dataSize, downloadBaud);
	delay(50);
	serial.begin(downloadBaud);

	return readAndDiscard(500, false);
}


void TJCDownload::startState(void) {
	stateHandler = &TJCDownload::waitConnectState;
}


bool TJCDownload::readAndDiscard(uint32_t timeoutMs, bool exitAfter05) {
	unsigned long startMs = millis();
	bool have05 = false;

	while(millis() - startMs < timeoutMs) {
		int c = serial.read();
		if (c != -1) {
			if (c == 0x05) {
				have05 = true;
				if (exitAfter05) {
					break;
				}

			}
		}
	}
	return have05;
}


void TJCDownload::waitConnectState(void) {
	if (WiFi.ready()) {
		requestCheck();
	}
}


void TJCDownload::requestCheck() {

	isDone = false;

	if (buffer == NULL) {
		buffer = (char *) malloc(BUFFER_SIZE);
		if (buffer == NULL) {
			Log.info("could not allocate buffer");
			stateHandler = &TJCDownload::cleanupState;
			return;
		}
	}

	// Connect to server by TCP
	if (client.connect(hostname, port)) {
		// Connected by TCP

		char ifModifiedSince[64];
		ifModifiedSince[0] = 0;

		// If we're downloading the json schema we only want a new file
		// If we're downloading firmware we're going to do it no matter what
		if (downloadType == JSON_SCHEMA && strlen(jsonModifiedDate)) {
			snprintf(ifModifiedSince, sizeof(ifModifiedSince), "If-Modified-Since: %s GMT\r\n", jsonModifiedDate);
			Log.info("If-Modified-Since %s", jsonModifiedDate);
		}
		else {
			Log.info("no last modification date");
		}

		// Send request header
		size_t count = snprintf(buffer, BUFFER_SIZE,
				"GET %s/%s HTTP/1.1\r\n"
				"Host: %s\r\n"
				"%s"
				"Connection: close\r\n"
				"\r\n",
				downloadType == JSON_SCHEMA ? jsonPath : firmwarePath,
				downloadType == JSON_SCHEMA ? jsonFilename : availableFirmwareFilename,
				hostname,
				ifModifiedSince
				);

		client.write((const uint8_t *)buffer, count);

		bufferOffset = 0;

		Log.info("sent request to %s:%d", hostname, port);

		stateTime = millis();
		stateHandler = &TJCDownload::headerWaitState;
	}
	else {
		Log.info("failed to connect to %s:%d", hostname, port);
		stateTime = millis();
		stateHandler = &TJCDownload::retryWaitState;
	}
}


void TJCDownload::headerWaitState(void) {
	if (!client.connected()) {
		Log.info("server disconnected unexpectedly");
		stateTime = millis();
		stateHandler = &TJCDownload::retryWaitState;
		return;
	}
	
	if (millis() - stateTime >= DATA_TIMEOUT_TIME_MS) {
		Log.info("timed out waiting for response header");
		client.stop();

		stateTime = millis();
		stateHandler = &TJCDownload::retryWaitState;
		return;
	}
	// Read some data
	int count = client.read((uint8_t *)&buffer[bufferOffset], BUFFER_SIZE - bufferOffset);
	if (count > 0) {

		Log.info("bufferOffset=%d count=%d", bufferOffset, count);

		bufferOffset += count;
		buffer[bufferOffset] = 0;

		char *end = strstr(buffer, "\r\n\r\n");
		if (end != NULL) {
			// Have a complete response header
			*end = 0;

			// Check status code, namely 200 (OK), 304 (not modified) or any other error
			{
				int code = 0;

				char *cp = strchr(buffer, ' ');
				if (cp) {
					cp++;
					code = atoi(cp);
				}
				if (code == 304) {
					Log.info("file not modified, not downloading again");
					stateHandler = &TJCDownload::cleanupState;
					return;
				}

				if (code != 200) {
					Log.info("not an OK response, was %d", code);
					stateHandler = &TJCDownload::cleanupState;
					return;
				}
			}


			if (downloadType == JSON_SCHEMA) {
				// Note the data from the Last-Modified header
				// Last-Modified: <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
				// Last-Modified: Wed, 21 Oct 2015 07:28:00 GMT
				char *cp = strstr(buffer, "Last-Modified:");
				if (cp) {
					cp += 14; // length of "Last-Modified:"

					// Skip the space after the Last-Modified
					if (*cp == ' ') {
						cp++;
					}

					size_t ii = 0;
					while(*cp != '\r' && ii < (DATE_BUFFER_SIZE - 1)) {
						jsonModifiedDate[ii++] = *cp++;
					}
					jsonModifiedDate[ii] = 0;

					Log.info("last modified: %s", jsonModifiedDate);
				}
			}

			// Note the data size from the Content-Length. This is required as the tjc protocol requires
			// the length before sending segments and we don't have enough RAM to buffer it first.
			{
				char *cp = strstr(buffer, "Content-Length:");
				if (cp) {
					cp += 15; // length of "Content-Length:"
					while(*cp == ' ') {
						cp++;
					}
					dataSize = atoi(cp);
				}
			}
			if (dataSize == 0) {
				Log.info("unable to get length of data");
				stateHandler = &TJCDownload::cleanupState;
				return;
			}


			dataOffset = 0;

			// Send the request to start downloading to the display
			if (downloadType == TFT_FIRMWARE && !startDownload()) {
				Log.info("display did not acknowledge download start");
				stateHandler = &TJCDownload::cleanupState;
				return;
			}

			Log.info("downloading %d bytes", dataSize);

			// Discard the header
			end += 4; // the \r\n\r\n part

			size_t newLength = bufferOffset - (end - buffer);
			if (newLength > 0) {
				memmove(buffer, end, newLength);
			}
			bufferOffset = newLength;
			stateTime = millis();
			stateHandler = &TJCDownload::dataWaitState;
		}
	}
}


void TJCDownload::dataWaitState(void) {
	if (!client.connected()) {
		Log.info("server disconnected unexpectedly");
		stateTime = millis();
		stateHandler = &TJCDownload::retryWaitState;
		return;
	}
	if (millis() - stateTime >= DATA_TIMEOUT_TIME_MS) {
		Log.info("timed out waiting for data");
		client.stop();

		stateTime = millis();
		stateHandler = &TJCDownload::retryWaitState;
		return;
	}

	// This is the amount of data that's left to read from the server
	size_t dataLeft = dataSize - dataOffset;
	if (dataLeft > BUFFER_SIZE) {
		dataLeft = BUFFER_SIZE;
	}

	// This part takes into account that we may have partial data in buf already (bufferOffset bytes)
	size_t requestSize = dataLeft;
	if (requestSize > (BUFFER_SIZE - bufferOffset)) {
		requestSize = BUFFER_SIZE - bufferOffset;
	}

	int count = client.read((uint8_t *)&buffer[bufferOffset], requestSize);
	if (count > 0) {
		bufferOffset += count;
		stateTime = millis();

		if (bufferOffset == dataLeft) {
			  // Got a whole buffer of data (or last partial buffer), send to display

			if (downloadType == TFT_FIRMWARE) {
				uint8_t retryCount = 0;
				while (true) {
					Log.info("sending to display dataOffset=%d dataSize=%d", dataOffset, dataSize);
					serial.write((const uint8_t *)buffer, bufferOffset);

  				  	// Wait for the display to acknowledge
				  	if (!readAndDiscard(500, true)) {
						Log.info("display did not acknowledge block - Retry %d", ++retryCount);
						if (retryCount >= 3) {
							stateHandler = &TJCDownload::cleanupState;
							return;
						}
				  	} else {
						break;
				  	}
			  	}
			}

			dataOffset += bufferOffset;
			bufferOffset = 0;
			if (dataOffset >= dataSize) {
				Log.info("successfully downloaded");
                isSuccess = true; // Won't always be successful here but this will do for now.
				
				if (downloadType == JSON_SCHEMA) {
					// Allocate the JSON document
					// Use arduinojson.org/v6/assistant to compute the capacity.
					const size_t capacity = 300;//JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
					DynamicJsonDocument doc(capacity);

					// Parse JSON object
					DeserializationError error = deserializeJson(doc, buffer);
					if (error) {
						Serial.print(F("deserializeJson() failed: "));
						Serial.println(error.c_str());
					} else {
						// Extract values
						//   Serial.println(F("Response:"));
						if (doc.containsKey(deviceSerialNumber)) {
                            JsonObject device = doc[deviceSerialNumber];
							strcpy(availableFirmwareFilename, device["filename"]);
							strcpy(availableFirmwareVersion, device["version"]);
							Log.info(availableFirmwareFilename);
							Log.info(availableFirmwareVersion);
                            if (strcmp(availableFirmwareVersion, deviceVersionNumber) == 0) {
                                Log.info("No update available");
                            } else {
                                Log.info("Version %s is available", availableFirmwareVersion);
								updateAvailable = true;
                            }
							// Log.info(device["project"].as<char*>());
							// Log.info(device["version"].as<char*>());
							// Log.info(device["filename"].as<char*>());
						} else {
							Log.info("serial number not found in json");
						}
					}
				}

				stateHandler = &TJCDownload::cleanupState;
				stateTime = millis();
			}
		}
	}
}


void TJCDownload::retryWaitState(void) {
	if (millis() - stateTime >= RETRY_WAIT_TIME_MS) {
		stateHandler = &TJCDownload::waitConnectState;
	}
}


void TJCDownload::cleanupState(void) {
	if (buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}
	client.stop();
	stateHandler = &TJCDownload::doneState;
}


void TJCDownload::doneState(void) {
	if (!isDone) {
		Log.info("done");
		isDone = true;
	} else {
		stateHandler = NULL;
	}
}
