#include "nodeStats.h"

NodeStats::NodeStats(TJC* tjc) { this->tjc = tjc; }

void NodeStats::setStat(uint8_t node, Stats stat, uint8_t value) {
  char rawValue[4];
  sprintf(rawValue, "%d", value);
  setStat(node, stat, rawValue);
}

void NodeStats::setStat(uint8_t node, Stats stat, const char* rawValue) {
  if (node >= 3)
    return;

  switch (stat) {
    case Uptime: {
      uint32_t value = strtoull(rawValue, NULL, 0);
      if (uptime[node] != value) {
        uptime[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case CPUPercent: {
      uint8_t value = strtoull(rawValue, NULL, 0);
      if (cpuPercent[node] != value) {
        cpuPercent[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case MemoryFree: {
      uint64_t value = strtoull(rawValue, NULL, 0);
      if (memFree[node] != value) {
        memFree[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case MemoryUsed: {
      uint64_t value = strtoull(rawValue, NULL, 0);
      if (memUsed[node] != value) {
        memUsed[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case MemoryTotal: {
      uint64_t value = strtoull(rawValue, NULL, 0);
      if (memTotal[node] != value) {
        memTotal[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case MemoryPercent: {
      uint8_t value = strtoull(rawValue, NULL, 0);
      if (memPercent[node] != value) {
        memPercent[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case SwapPercent: {
      uint8_t value = strtoull(rawValue, NULL, 0);
      if (swapPercent[node] != value) {
        swapPercent[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case Load1: {
      double value = atof(rawValue);
      if (load1[node] != value) {
        load1[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case Load5: {
      double value = atof(rawValue);
      if (load5[node] != value) {
        load5[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case Load15: {
      double value = atof(rawValue);
      if (load15[node] != value) {
        load15[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case TemperatureCore0: {
      uint8_t value = strtoull(rawValue, NULL, 0);
      if (temperatureCore0[node] != value) {
        temperatureCore0[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case TemperatureCore1: {
      uint8_t value = strtoull(rawValue, NULL, 0);
      if (temperatureCore1[node] != value) {
        temperatureCore1[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case NicTX: {
      uint64_t value = strtoull(rawValue, NULL, 0);
      if (nicTX[node] != value) {
        nicTX[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case NicRX: {
      uint64_t value = strtoull(rawValue, NULL, 0);
      if (nicRX[node] != value) {
        nicRX[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case FS1Used: {
      uint64_t value = strtoull(rawValue, NULL, 0);
      if (fs1Used[node] != value) {
        fs1Used[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case FS1Total: {
      uint64_t value = strtoull(rawValue, NULL, 0);
      if (fs1Total[node] != value) {
        fs1Total[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case FS2Used: {
      uint64_t value = strtoull(rawValue, NULL, 0);
      if (fs2Used[node] != value) {
        fs2Used[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case FS2Total: {
      uint64_t value = strtoull(rawValue, NULL, 0);
      if (fs2Total[node] != value) {
        fs2Total[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
    case DockerContainers: {
      uint8_t value = strtoull(rawValue, NULL, 0);
      if (dockerContainers[node] != value) {
        dockerContainers[node] = value;
        updateDisplay(node, stat);
      }
      break;
    }
  }
}

void NodeStats::updateDisplay(uint8_t node, Stats stat) {
    if (!tjc->getIsReady())
        return;

  if (tjc->getPage() == 1) {
    switch (stat) {
        case Uptime:
            setUptimeText(node, uptime[node]);
            break;
        case CPUPercent:
            setTextPercent(node, "cpu", cpuPercent[node]);
            setProgressBar(node, "cpu", cpuPercent[node]);
            break;
        case MemoryFree:
            setText(node, "memfree", bytesToHumanSize(memFree[node]));
            break;
        case MemoryUsed:
            setText(node, "memused", bytesToHumanSize(memUsed[node]));
            break;
        case MemoryTotal:
            setText(node, "memtotal", bytesToHumanSize(memTotal[node]));
            break;
        case MemoryPercent:
            setTextPercent(node, "mem", memPercent[node]);
            setProgressBar(node, "mem", memPercent[node]);
            break;
        case SwapPercent:
            setTextPercent(node, "swap", swapPercent[node]);
            setProgressBar(node, "swap", swapPercent[node]);
            break;
        case Load1:
            setTextDouble(node, "load1", load1[node]);
            break;
        case Load5:
            setTextDouble(node, "load5", load5[node]);
            break;
        case Load15:
            setTextDouble(node, "load15", load15[node]);
            break;
        case TemperatureCore0:
            setTextTemperature(node, "core0", temperatureCore0[node]);
            break;
        case TemperatureCore1:
            setTextTemperature(node, "core1", temperatureCore1[node]);
            break;
        case NicTX:
            setText(node, "nictx", bytesToHumanSize(nicTX[node]));
            break;
        case NicRX:
            setText(node, "nicrx", bytesToHumanSize(nicRX[node]));
            break;
        case FS1Used:
            setText(node, "fs1used", bytesToHumanSize(fs1Used[node]));
            break;
        case FS1Total:
            setText(node, "fs1total", bytesToHumanSize(fs1Total[node]));
            break;
        case FS2Used:
            setText(node, "fs2used", bytesToHumanSize(fs2Used[node]));
            break;
        case FS2Total:
            setText(node, "fs2total", bytesToHumanSize(fs2Total[node]));
            break;
        case DockerContainers:
            setText(node, "container", dockerContainers[node]);
            break;
    }
  } else if (tjc->getPage() == 2 && currentNode == node) {
    switch (stat) {
        case CPUPercent:
        {
            char buffer[5];
            sprintf(buffer, "%d%%", cpuPercent[currentNode]);
            tjc->setText("txtcpu", buffer);
            uint8_t pic = cpuPercent[currentNode] / 5;
            tjc->setPic("pcpu", pic);
        }
            break;
        case MemoryPercent:
        {
            char buffer[5];
            sprintf(buffer, "%d%%", memPercent[currentNode]);
            tjc->setText("txtmem", buffer);
            uint8_t pic = 21 + (memPercent[currentNode] / 5);
            tjc->setPic("pmem", pic);
        }
            break;
        case SwapPercent:
        {
            char buffer[5];
            sprintf(buffer, "%d%%", swapPercent[currentNode]);
            tjc->setText("txtswap", buffer);
            uint8_t pic = 42 + (swapPercent[currentNode] / 5);
            tjc->setPic("pswap", pic);
        }
            break;
    }
  }
}

void NodeStats::fullPageRefresh() {
    if (tjc->getPage() == 1) {
        for (uint8_t node = 0; node < 3; node++) {
            updateDisplay(node, Uptime);
            updateDisplay(node, CPUPercent);
            updateDisplay(node, MemoryFree);
            updateDisplay(node, MemoryUsed);
            updateDisplay(node, MemoryTotal);
            updateDisplay(node, MemoryPercent);
            updateDisplay(node, SwapPercent);
            updateDisplay(node, Load1);
            updateDisplay(node, Load5);
            updateDisplay(node, Load15);
            updateDisplay(node, TemperatureCore0);
            updateDisplay(node, TemperatureCore1);
            updateDisplay(node, NicTX);
            updateDisplay(node, NicRX);
            updateDisplay(node, FS1Used);
            updateDisplay(node, FS1Total);
            updateDisplay(node, FS2Used);
            updateDisplay(node, FS2Total);
            updateDisplay(node, DockerContainers);
        }
    } else if (tjc->getPage() == 2) {
        updateDisplay(currentNode, CPUPercent);
        updateDisplay(currentNode, MemoryPercent);
        updateDisplay(currentNode, SwapPercent);
    }
}


void NodeStats::setTextDouble(const uint8_t node, const char* field, double value) {
  char text[100];
  sprintf(text, "%0.2f", value);
  setText(node, field, text);
}


void NodeStats::setTextTemperature(const uint8_t node, const char* field, uint8_t value) {
  char text[100];
  sprintf(text, "%dC", value);
  setText(node, field, text);
}

void NodeStats::setTextPercent(const uint8_t node, const char* field, uint8_t value) {
  char text[100];
  sprintf(text, "%d%%", value);
  setText(node, field, text);
}


void NodeStats::setText(const uint8_t node, const char* obj, const uint8_t value) {
  char txtValue[4];
  sprintf(txtValue, "%d", value);
  setText(node, obj, txtValue);
}


void NodeStats::setText(const uint8_t node, const char* obj, const char* value) {
  char objName[15];
  sprintf(objName, "s%dtxt%s", node, obj);
  tjc->setText(objName, value);
}


void NodeStats::setUptimeText(const uint8_t node, uint64_t uptime) {
  char objName[15];
  char value[17];

  sprintf(objName, "s%dtxtuptime", node);

  int days = uptime / (24 * 3600);
  uptime = uptime % (24 * 3600);
  int hours = uptime / 3600;
  uptime %= 3600;
  int minutes = uptime / 60;
  
  if (days > 0) {
    sprintf(value, "Uptime %dd, %02d:%02d", days, hours, minutes);
  } else {
    sprintf(value, "Uptime %02d:%02d", hours, minutes);
  }

  tjc->setText(objName, value);
}


void NodeStats::setProgressBar(uint8_t node, const char* obj, uint8_t value) {

    char objName[15];
    sprintf(objName, "s%dpb%s", node, obj);

    if (value < 75) {
      // Go for blue
      tjc->setForegroundColor(objName, progressBarColors[0]);
    } else if (value >= 75 && value < 90) {
      // Go for yellow
      tjc->setForegroundColor(objName, progressBarColors[1]);
    } else {
      // Go for red
      tjc->setForegroundColor(objName, progressBarColors[2]);
    }

    tjc->setProgressBar(objName, value);
}


const char* NodeStats::bytesToHumanSize(uint64_t bytes) {

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