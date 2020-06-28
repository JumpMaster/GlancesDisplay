#ifndef NodeStats_h
#define NodeStats_h

#include "Particle.h"
#include "nextion.h"

class NodeStats
{
public:
  typedef enum {
      Uptime = 0,
      CPUPercent = 1,
      MemoryFree = 2,
      MemoryUsed = 3,
      MemoryTotal = 4,
      MemoryPercent = 5,
      SwapPercent = 6,
      Load1 = 7,
      Load5 = 8,
      Load15 = 9,
      TemperatureCore0 = 10,
      TemperatureCore1 = 11,
      NicTX = 12,
      NicRX = 13,
      FS1Used = 14,
      FS1Total = 15,
      FS2Used = 16,
      FS2Total = 17,
      DockerContainers = 18,
  } Stats;

  NodeStats(Nextion* nextion);
  void setStat(uint8_t node, Stats stat, const char* rawValue);
  void setStat(uint8_t node, Stats stat, uint8_t value);
protected:
  Nextion* nextion;

  void setUptimeText(const uint8_t node, uint64_t uptime);
  void setText(const uint8_t node, const char* objName, const uint8_t value);
  void setText(const uint8_t node, const char* objName, const char* value);
  void setTextDouble(const uint8_t node, const char* field, double value);
  void setTextPercent(const uint8_t node, const char* field, uint8_t value);
  void setTextTemperature(const uint8_t node, const char* field, uint8_t value);
  void setProgressBar(uint8_t server, const char* field, uint8_t value);
  void updateDisplay(uint8_t node, Stats stat);

  const char* bytesToHumanSize(uint64_t bytes);

  const uint16_t progressBarColors[3] = {9407, 64677, 64000};

  uint32_t uptime[3];
  uint8_t cpuPercent[3];
  uint64_t memFree[3];
  uint64_t memUsed[3];
  uint64_t memTotal[3];
  uint8_t memPercent[3];
  uint8_t swapPercent[3] = {255, 255, 255};
  double load1[3];
  double load5[3];
  double load15[3];
  uint8_t temperatureCore0[3];
  uint8_t temperatureCore1[3];
  uint64_t nicTX[3];
  uint64_t nicRX[3];
  uint64_t fs1Used[3];
  uint64_t fs1Total[3];
  uint64_t fs2Used[3];
  uint64_t fs2Total[3];
  uint8_t dockerContainers[3];
};

#endif