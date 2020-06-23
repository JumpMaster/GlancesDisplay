#include "display.h"

Display::Display(Nextion* nextion) { this->nextion = nextion; }

void Display::setProgressBar(uint8_t server, Field field, uint8_t value) {
    
    uint8_t *previousValue;

    char progressBar[9];
    char textField[10];

    if (field == CPU) {
        previousValue = &cpuPct[server];
        sprintf(progressBar, "s%dpbcpu", server+1);
        sprintf(textField, "s%dtxtcpu", server+1);

    } else if (field == Memory) {
        previousValue = &memoryPct[server];
        sprintf(progressBar, "s%dpbmem", server+1);
        sprintf(textField, "s%dtxtmem", server+1);
    } else {
        previousValue = &swapPct[server];
        sprintf(progressBar, "s%dpbswap", server+1);
        sprintf(textField, "s%dtxtswap", server+1);
    }

    // if (*previousValue != value) {
    if (value < 75) {
        if (*previousValue >= 75) {
            // Go for blue
            nextion->setForegroundColor(progressBar, progressBarColors[0]);
        }
    } else if (value >= 75 && value < 90) {
        if (*previousValue < 75 || *previousValue >= 90) {
            // Go for yellow
            nextion->setForegroundColor(progressBar, progressBarColors[1]);
        }
    } else if (*previousValue < 90) {
        // Go for red
        nextion->setForegroundColor(progressBar, progressBarColors[2]);
    }

    nextion->setProgressBar(progressBar, value);
    char valuePct[5];
    sprintf(valuePct, "%d%%", value);
    nextion->setText(textField, valuePct);

    *previousValue = value;
}

void Display::setByteText(const uint8_t server, const char* name, const char* value) {
  char field[100];
  sprintf(field, "s%dtxt%s", server+1, name);
  nextion->setText(field, bytesToHumanSize(value));
}

void Display::setText(const uint8_t server, const char* name, const uint8_t value) {
    char strValue[100];
    sprintf(strValue, "%d", value);
    setText(server, name, strValue);
}

void Display::setText(const uint8_t server, const char* name, const char* value) {
  char field[100];
  sprintf(field, "s%dtxt%s", server+1, name);
  nextion->setText(field, value);
}

void Display::setTemperatureText(const uint8_t server, const char* name, const char* value) {
  char field[100];
  char data[100];
  sprintf(field, "s%dtxt%s", server+1, name);
  sprintf(data, "%sC", value);
  nextion->setText(field, data);
}

void Display::setUptimeText(const uint8_t server, int uptime) {
  char field[100];
  char value[100];

  int days = uptime / (24 * 3600);
  uptime = uptime % (24 * 3600);
  int hours = uptime / 3600;
  uptime %= 3600;
  int minutes = uptime / 60;
  sprintf(field, "s%dtxtuptime", server+1);
  if (days > 0) {
      sprintf(value, "Uptime %dd, %02d:%02d", days, hours, minutes);
  } else {
      sprintf(value, "Uptime %02d:%02d", hours, minutes);
  }

  nextion->setText(field, value);
}







const char* Display::bytesToHumanSize(const char* cBytes) {
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