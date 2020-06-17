#include "mqtt.h"
#include "nextion.h"
#include "papertrail.h"
#include "secrets.h"

//  Stubs
void mqttCallback(char* topic, byte* payload, unsigned int length);

PapertrailLogHandler papertrailHandler(papertrailAddress, papertrailPort,
  "ArgonGlances", System.deviceID(),
  LOG_LEVEL_NONE, {
  { "app", LOG_LEVEL_ALL }
  // TOO MUCH!!! { “system”, LOG_LEVEL_ALL },
  // TOO MUCH!!! { “comm”, LOG_LEVEL_ALL }
});

ApplicationWatchdog wd(60000, System.reset);

MQTT mqttClient(mqttServer, 1883, mqttCallback);
uint32_t lastMqttConnectAttempt;
const int mqttConnectAtemptTimeout1 = 5000;
const int mqttConnectAtemptTimeout2 = 30000;
unsigned int mqttConnectionAttempts;
uint32_t resetTime = 0;
retained uint32_t lastHardResetTime;
retained int resetCount;
char serialData[100];
uint8_t serialPosition;

uint8_t dockerContainerCount[3];
uint8_t dockerContainers[3];
uint32_t containerUpdateTimeout[3];

Nextion nextion(Serial1);

int startUpdate(const char* data) {
  bool force = false;
  if (strcmp(data, "force") == 0) {
    force = true;
  }
  nextion.doUpdate(force);
  return 0;
}

int setPower(const char* data) {
  if (strcmp(data, "on") == 0) {
    nextion.powerOn();
    return 1;
  } else {
    nextion.powerOff();
    return 0;
  }
}

static const char* bytesToHumanSize(const char* cBytes) {
  uint64_t bytes = strtoull(cBytes, NULL, 0);

	char const *suffix[] = {"B", "K", "M", "G", "T"};
	char length = sizeof(suffix) / sizeof(suffix[0]);

	int i = 0;
	double dblBytes = bytes;

	if (bytes > 1024) {
		for (i = 0; (bytes / 1024) > 0 && i<length-1; i++, bytes /= 1024)
			dblBytes = bytes / 1024.0;
	}

	static char output[200];
	sprintf(output, "%.01lf%s", dblBytes, suffix[i]);
	return output;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {

  // if (firmwareUpdateInProgress)
    // return;

    char p[length + 1];
    memcpy(p, payload, length);
    p[length] = '\0';
    // Log.info("%s - %s", topic, p);
    
    char t[strlen(topic)];
    memcpy(t, topic, strlen(topic));
        
    char topics[10][30];
    
    const char* delim = "/";
    uint8_t topicLevels = 0;
    char *ptr = strtok(t, delim);
    while (ptr != NULL) {
      strcpy(topics[topicLevels++], ptr);
      ptr = strtok(NULL, delim);
    }

    uint8_t server = 0;
    if (strcmp(topics[1], "andromeda") == 0) {
      server = 1;
    } else if (strcmp(topics[1], "pinwheel") == 0) {
      server = 2;
    } else if (strcmp(topics[1], "qnap") == 0) {
      server = 3;
    } else {
      return;
    }

    if (strcmp(topics[2], "cpu") == 0) {
      uint8_t cpu = atoi(p);
      nextion.setProgressBar(1, server, 1, cpu);
      nextion.setText(1, server, "cpu", cpu, "%");
    } else if (strcmp(topics[2], "mem") == 0) {
      if (strncmp(topics[3], "percent", 7) == 0) {
        uint8_t mem = atoi(p);
        nextion.setProgressBar(1, server, 2, mem);
        nextion.setText(1, server, "memory", mem, "%");
      } else if (strncmp(topics[3], "total", 5) == 0) {
        nextion.setText(1, server, "memtotal", bytesToHumanSize(p));
      } else if (strncmp(topics[3], "used", 4) == 0) {
        nextion.setText(1, server, "memused", bytesToHumanSize(p));
      } else if (strncmp(topics[3], "free", 4) == 0) {
        nextion.setText(1, server, "memfree", bytesToHumanSize(p));
      }
    } else if (strcmp(topics[2], "memswap") == 0) {
      uint8_t swap = atoi(p);
      nextion.setProgressBar(1, server, 3, swap);
      nextion.setText(1, server, "swap", swap, "%");
    } else if (strcmp(topics[2], "uptime") == 0) {
      int uptime = atoi(p);
      nextion.setUptimeText(1, server, uptime);
    } else if (strcmp(topics[2], "load") == 0) {
      if (strncmp(topics[3], "min15", 5) == 0) {
        nextion.setText(1, server, "load15", p);
      } else if (strncmp(topics[3], "min5", 4) == 0) {
        nextion.setText(1, server, "load5", p);
      } else if (strncmp(topics[3], "min1", 4) == 0) {
        nextion.setText(1, server, "load1", p);
      }
    } else if (strcmp(topics[2], "sensors") == 0) {
      if (strncmp(topics[4], "value", 5) == 0) {
        if (strcmp(topics[3], "Core_0") == 0) {
          nextion.setText(1, server, "core0", p, "C");
        } else if (strcmp(topics[3], "Core_1") == 0) {
          nextion.setText(1, server, "core1", p, "C");
        }
      }
    } else if (strcmp(topics[2], "docker") == 0) {
      if (strncmp(topics[topicLevels-1], "key", 3) == 0) {
        dockerContainerCount[server-1]++;
        containerUpdateTimeout[server-1] = millis()+1000;
      }
    } else if (strcmp(topics[2], "network") == 0) {
      if (strcmp(topics[3], "eno1") == 0 || strcmp(topics[3], "eth0") == 0) {
        if (strncmp(topics[4], "tx", 2) == 0) {
          nextion.setText(1, server, "nictx", bytesToHumanSize(p));
        } else if (strncmp(topics[4], "rx", 2) == 0) {
          nextion.setText(1, server, "nicrx", bytesToHumanSize(p));
        }
      }
    } else if (strcmp(topics[2], "fs") == 0) {
      if (strcmp(topics[3], "_share_CACHEDEV1_DATA") == 0 ||
          strcmp(topics[3], "_") == 0) {
        if (strncmp(topics[4], "size", 4) == 0) {
          nextion.setText(1, server, "fs1total", bytesToHumanSize(p));
        } else if (strncmp(topics[4], "used", 4) == 0) {
          nextion.setText(1, server, "fs1used", bytesToHumanSize(p));
        }        
      } else if (strcmp(topics[3], "_gluster") == 0) {
        if (strncmp(topics[4], "size", 4) == 0) {
          nextion.setText(1, server, "fs2total", bytesToHumanSize(p));
        } else if (strncmp(topics[4], "used", 4) == 0) {
          nextion.setText(1, server, "fs2used", bytesToHumanSize(p));
        }        
      }
    }
}

void connectToMQTT() {
    lastMqttConnectAttempt = millis();
    bool mqttConnected = mqttClient.connect(System.deviceID(), mqttUsername, mqttPassword);
    if (mqttConnected) {
        mqttConnectionAttempts = 0;
        Log.info("MQTT Connected");
        mqttClient.subscribe("glances/+/cpu/total");
        mqttClient.subscribe("glances/+/mem/+");
        mqttClient.subscribe("glances/+/load/+");
        mqttClient.subscribe("glances/+/memswap/percent");
        mqttClient.subscribe("glances/+/fs/+/size");
        mqttClient.subscribe("glances/+/fs/+/used");
        mqttClient.subscribe("glances/+/uptime/seconds");
        mqttClient.subscribe("glances/+/sensors/+/value");
        mqttClient.subscribe("glances/+/network/+/rx");
        mqttClient.subscribe("glances/+/network/+/tx");
        mqttClient.subscribe("glances/+/docker/#");
    } else {
        mqttConnectionAttempts++;
        Log.info("MQTT failed to connect");
    }
}

int runNextionCommand(const char* data) {
  nextion.run(data);
  return 0;
}

SYSTEM_THREAD(ENABLED);

void startupMacro() {
    System.enableFeature(FEATURE_RESET_INFO);
    System.enableFeature(FEATURE_RETAINED_MEMORY);
}
STARTUP(startupMacro());

void setup(void) {
    nextion.setup();
    nextion.powerOff();

    waitFor(Particle.connected, 30000);
    
    do {
        resetTime = Time.now();
        Particle.process();
    } while (resetTime < 1500000000 || millis() < 10000);
    
    if (System.resetReason() == RESET_REASON_PANIC) {
        if ((Time.now() - lastHardResetTime) < 120) {
            resetCount++;
        } else {
            resetCount = 1;
        }

        lastHardResetTime = Time.now();

        if (resetCount > 3) {
            System.enterSafeMode();
        }
    } else {
        resetCount = 0;
    }

    Particle.function("run", runNextionCommand);
    Particle.function("startFirmwareUpdate", startUpdate);
    Particle.function("setPower", setPower);

    Log.info("Boot complete. Reset count = %d", resetCount);
    nextion.powerOn();
}

void loop() {
  nextion.loop();

  if (nextion.getPage() == 0 && nextion.getIsReady()) {
    nextion.setPage(1);
  }

  for (int i = 0; i < 3; i++) {
    if (containerUpdateTimeout[i] > 0 && millis() > containerUpdateTimeout[i]) {
      containerUpdateTimeout[i] = 0;
      dockerContainers[i] = dockerContainerCount[i];
      dockerContainerCount[i] = 0;
      nextion.setText(1, i+1, "container", dockerContainers[i]);
    }
  }
  if (nextion.getIsReady()) {
    if (mqttClient.isConnected()) {
        mqttClient.loop();
    } else if ((mqttConnectionAttempts < 5 && millis() > (lastMqttConnectAttempt + mqttConnectAtemptTimeout1)) ||
                  millis() > (lastMqttConnectAttempt + mqttConnectAtemptTimeout2)) {
        connectToMQTT();
    }
  } else if (mqttClient.isConnected()) {
    mqttClient.disconnect();
  }

  wd.checkin();  // resets the AWDT count
}