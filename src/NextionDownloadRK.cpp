#include "NextionDownloadRK.h"


// https://www.itead.cc/blog/nextion-hmi-upload-protocol

NextionDownload::NextionDownload(){}//USARTSerial &serial, int eepromLocation) : serial(serial), eepromLocation(eepromLocation)  {
//}

NextionDownload::~NextionDownload() {
}

void NextionDownload::setup() {
	isDone = false;
	hasRun = false;
	stateHandler = &NextionDownload::startState;
	// delay(4000);
	// tryBaud(9600);
}


void NextionDownload::loop() {
	if (stateHandler != NULL) {
		stateHandler(*this);
	}
}
/*
bool NextionDownload::testDisplay() {

	return findBaud();
}
*/

size_t NextionDownload::readData(char *buf, size_t bufSize, uint32_t timeoutMs, bool exitAfter05) {
	unsigned long startMs = millis();
	size_t count = 0;

	while(millis() - startMs < timeoutMs) {
		int c = serial.read();
		if (c != -1) {
			if (buf && count < bufSize) {
				buf[count] = (char) c;
			}
			count++;

			if (exitAfter05 && c == 0x05) {
				break;
			}
		}
	}
	// Make sure buf is null-terminated since we use strstr on it
	if (count < (bufSize -1)) {
		buf[count] = 0;
	}
	else {
		buf[bufSize - 1] = 0;
	}

	return count;
}

bool NextionDownload::readAndDiscard(uint32_t timeoutMs, bool exitAfter05) {
	unsigned long timeoutMillis = millis() + timeoutMs;
	bool have05 = false;

	while(millis() < timeoutMillis) {
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

void NextionDownload::readAvailableAndDiscard() {
	while(serial.available()) {
		(void) serial.read();
	}
}

void NextionDownload::sendCommand(const char *fmt, ...) {
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
/*
bool NextionDownload::tryBaud(int baud) {
	serial.begin(baud);

	sendCommand("");
	sendCommand("connect");

	// Read
	char buf[128];
	readData(buf, sizeof(buf), 100, false);

	bool result = strstr(buf, "comok") != 0;

	Log.info("tryBaud %d: %d", baud, result);

	return result;
}
*/
/*
bool NextionDownload::findBaud() {
	static const int bauds[] = {9600,115200,19200,57600,38400,4800,2400};
	bool result = false;

	for(size_t ii = 0; ii < sizeof(bauds)/sizeof(bauds[0]); ii++) {
		result = tryBaud(bauds[ii]);
		if (result) {
			break;
		}
	}

	if (!result) {
		// Reset to 9600 if not found
		serial.begin(9600);
	}
	return result;
}
*/
bool NextionDownload::startDownload() {

	Log.info("start download dataSize=%d downloadBaud=%d", dataSize, downloadBaud);

	sendCommand("");
	sendCommand("whmi-wri %d,%d,0", dataSize, downloadBaud);
	delay(50);
	serial.begin(downloadBaud);

	return readAndDiscard(500, false);
}


bool NextionDownload::networkReady() {
	return WiFi.ready();
}


void NextionDownload::startState(void) {
	if (checkMode == CHECK_MODE_AT_BOOT) {
		stateHandler = &NextionDownload::waitConnectState;
	}
	else {
		stateHandler = &NextionDownload::doneState;
	}
}
void NextionDownload::waitConnectState(void) {
	// This is basically WiFi.ready() or Cellular.ready() depending
	if (networkReady()) {
		// We only get here when using checkMode == CHECK_MODE_AT_BOOT and network is ready

		requestCheck();//forceDownload);
	}
}

void NextionDownload::requestCheck() { //bool forceDownload /* = false */) {
	// this->forceDownload = forceDownload;

	isDone = false;
	hasRun = false;

	if (buffer == NULL) {
		buffer = (char *) malloc(BUFFER_SIZE);
		if (buffer == NULL) {
			Log.info("could not allocate buffer");
			stateHandler = &NextionDownload::cleanupState;
			return;
		}
	}

	// If we get this far, once we get to done state we can assume that we probably downloaded
	// firmware, or we gave up
	hasRun = true;

	// Make sure display can be found
	/*
	if (!findBaud()) {
		Log.info("could not detect display");
		stateHandler = &NextionDownload::cleanupState;
		return;
	}
	*/

	// Connect to server by TCP
	if (client.connect(hostname, port)) {
		// Connected by TCP

		char ifModifiedSince[64];
		ifModifiedSince[0] = 0;

		// char eepromBuffer[EEPROM_BUFFER_SIZE];
		// EEPROM.get(eepromLocation, eepromBuffer);
		// if (forceDownload) {
		// 	Log.info("forceDownload");
		// }
		// else
		// if (eepromBuffer[0] != 0xff) {
		// 	snprintf(ifModifiedSince, sizeof(ifModifiedSince), "If-Modified-Since: %s GMT\r\n", eepromBuffer);

		// 	Log.info("If-Modified-Since %s", eepromBuffer);
		// }
		// else {		char ifModifiedSince[64];
		// ifModifiedSince[0] = 0;

		// char eepromBuffer[EEPROM_BUFFER_SIZE];
		// EEPROM.get(eepromLocation, eepromBuffer);
		// if (forceDownload) {
		// 	Log.info("forceDownload");
		// }
		// else
		// if (eepromBuffer[0] != 0xff) {
		// 	snprintf(ifModifiedSince, sizeof(ifModifiedSince), "If-Modified-Since: %s GMT\r\n", eepromBuffer);

		// 	Log.info("If-Modified-Since %s", eepromBuffer);
		// }
		// else {
		// 	Log.info("no last modification date");
		// }
		// 	Log.info("no last modification date");
		// }

		// Send request header
		size_t count = snprintf(buffer, BUFFER_SIZE,
				"GET %s HTTP/1.1\r\n"
				"Host: %s\r\n"
				"%s"
				"Connection: close\r\n"
				"\r\n",
				pathPartOfUrl.c_str(),
				hostname.c_str(),
				ifModifiedSince
				);

		client.write((const uint8_t *)buffer, count);

		bufferOffset = 0;

		Log.info("sent request to %s:%d", hostname.c_str(), port);
		stateTime = millis();
		stateHandler = &NextionDownload::headerWaitState;
	}
	else {
		Log.info("failed to connect to %s:%d", hostname.c_str(), port);
		stateTime = millis();
		stateHandler = &NextionDownload::retryWaitState;
	}

}

void NextionDownload::headerWaitState(void) {
	if (!client.connected()) {
		Log.info("server disconnected unexpectedly");
		stateTime = millis();
		stateHandler = &NextionDownload::retryWaitState;
		return;
	}
	if (millis() - stateTime >= DATA_TIMEOUT_TIME_MS) {
		Log.info("timed out waiting for response header");
		client.stop();

		stateTime = millis();
		stateHandler = &NextionDownload::retryWaitState;
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
					stateHandler = &NextionDownload::cleanupState;
					return;
				}

				if (code != 200) {
					Log.info("not an OK response, was %d", code);
					stateHandler = &NextionDownload::cleanupState;
					return;
				}
			}


			{
				// Note the data from the Last-Modified header
				// Last-Modified: <day-name>, <day> <month> <year> <hour>:<minute>:<second> GMT
				// Last-Modified: Wed, 21 Oct 2015 07:28:00 GMT
				/*
				char *cp = strstr(buffer, "Last-Modified:");
				if (cp) {
					cp += 14; // length of "Last-Modified:"

					// Skip the space after the Last-Modified
					if (*cp == ' ') {
						cp++;
					}

					char eepromBuffer[EEPROM_BUFFER_SIZE];
					size_t ii = 0;
					while(*cp != '\r' && ii < (EEPROM_BUFFER_SIZE - 1)) {
						eepromBuffer[ii++] = *cp++;
					}
					eepromBuffer[ii] = 0;

					Log.info("last modified: %s", eepromBuffer);
					EEPROM.put(eepromLocation, eepromBuffer);
				}
				*/
			}

			// Note the data size from the Content-Length. This is required as the Nextion protocol requires
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
				stateHandler = &NextionDownload::cleanupState;
				return;
			}


			dataOffset = 0;

			// Send the request to start downloading to the display
			if (!startDownload()) {
				Log.info("display did not acknowledge download start");
				stateHandler = &NextionDownload::cleanupState;
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
			stateHandler = &NextionDownload::dataWaitState;
		}
	}

}

void NextionDownload::dataWaitState(void) {
	if (!client.connected()) {
		Log.info("server disconnected unexpectedly");
		stateTime = millis();
		stateHandler = &NextionDownload::retryWaitState;
		return;
	}
	if (millis() - stateTime >= DATA_TIMEOUT_TIME_MS) {
		Log.info("timed out waiting for data");
		client.stop();

		stateTime = millis();
		stateHandler = &NextionDownload::retryWaitState;
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

	// Log.info("bufferOffset=%d dataLeft=%d requestSize=%d dataOffset=%d", bufferOffset, dataLeft, requestSize, dataOffset);

	int count = client.read((uint8_t *)&buffer[bufferOffset], requestSize);
	if (count > 0) {
		bufferOffset += count;
		stateTime = millis();

		if (bufferOffset == dataLeft) {
			// Got a whole buffer of data (or last partial buffer), send to display

			uint8_t retryCount = 0;
			while (retryCount < 3) {
				Log.info("sending to display dataOffset=%d dataSize=%d", dataOffset, dataSize);
				serial.write((const uint8_t *)buffer, bufferOffset);

				// Wait for the display to acknowledge
				if (!readAndDiscard(500, true)) {
					Log.info("display did not acknowledge block - Retry %d", ++retryCount);
					if (retryCount >= 3) {
						stateHandler = &NextionDownload::cleanupState;
						return;
					}
				} else {
					break;
				}
			}

			dataOffset += bufferOffset;
			bufferOffset = 0;
			if (dataOffset >= dataSize) {
				Log.info("successfully downloaded");

				// Wait a few seconds for the display to restart
				stateHandler = &NextionDownload::restartWaitState;
				stateTime = millis();
			}
		}
	}
}

void NextionDownload::restartWaitState(void) {
	if (millis() - stateTime >= restartWaitTime) {
		// Reset the baud rate
		// findBaud();

		stateHandler = &NextionDownload::cleanupState;
	}
}

void NextionDownload::retryWaitState(void) {
	if (!retryOnFailure) {
		// Not retrying on failure (default), so just clean up
		stateHandler = &NextionDownload::cleanupState;
		return;
	}

	if (millis() - stateTime >= RETRY_WAIT_TIME_MS) {
		stateHandler = &NextionDownload::waitConnectState;
	}
}

void NextionDownload::cleanupState(void) {
	if (buffer != NULL) {
		free(buffer);
		buffer = NULL;
	}
	client.stop();

	stateHandler = &NextionDownload::doneState;
}


void NextionDownload::doneState(void) {
	if (!isDone) {
		Log.info("done");
		isDone = true;
	}
	// while (serial.available()) {
		// int byte = serial.read();
		// Log.info("%ld - %d", millis(), byte);
	// }
}