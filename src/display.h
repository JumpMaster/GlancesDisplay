#ifndef Display_h
#define Display_h

#include "Particle.h"
#include "nextion.h"

class Display
{
public:
  typedef enum {
      CPU = 0,
      Memory = 1,
      Swap = 3,
  } Field;

  Display(Nextion* nextion);
  void setProgressBar(uint8_t server, Field field, uint8_t value);
  void setByteText(const uint8_t server, const char* name, const char* value);
  void setText(const uint8_t server, const char* name, const uint8_t value);
  void setText(const uint8_t server, const char* name, const char* value);
  void setTemperatureText(const uint8_t server, const char* name, const char* value);
  void setUptimeText(const uint8_t server, int uptime);
protected:
  Nextion* nextion;
  const char* bytesToHumanSize(const char* cBytes);
  const char* fieldNames[3] = {"cpu", "memory", "swap"};
  uint8_t cpuPct[3] = {254, 254, 254};
  uint8_t memoryPct[3] = {254, 254, 254};
  uint8_t swapPct[3] = {254, 254, 254};
  const uint16_t progressBarColors[3] = {9407, 64677, 64000};
};

#endif