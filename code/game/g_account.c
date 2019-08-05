#include "g_local.h"

void TimeToString(int duration_ms, char *timeStr, size_t strSize, qboolean noMs) {
	if (duration_ms > (60 * 60 * 1000)) { //thanks, eternal
		int hours, minutes, seconds, milliseconds;
		hours = (int)((duration_ms / (1000 * 60 * 60))); //wait wut
		minutes = (int)((duration_ms / (1000 * 60)) % 60);
		seconds = (int)(duration_ms / 1000) % 60;
		milliseconds = duration_ms % 1000;
		if (noMs)
			Com_sprintf(timeStr, strSize, "%i:%02i:%02i", hours, minutes, seconds);
		else
			Com_sprintf(timeStr, strSize, "%i:%02i:%02i.%03i", hours, minutes, seconds, milliseconds);
	}
	else if (duration_ms > (60 * 1000)) {
		int minutes, seconds, milliseconds;
		minutes = (int)((duration_ms / (1000 * 60)) % 60);
		seconds = (int)(duration_ms / 1000) % 60;
		milliseconds = duration_ms % 1000;
		if (noMs)
			Com_sprintf(timeStr, strSize, "%i:%02i", minutes, seconds);
		else
			Com_sprintf(timeStr, strSize, "%i:%02i.%03i", minutes, seconds, milliseconds);
	}
	else {
		if (noMs)
			Q_strncpyz(timeStr, va("%.0f", ((float)duration_ms * 0.001)), strSize);
		else
			Q_strncpyz(timeStr, va("%.3f", ((float)duration_ms * 0.001)), strSize);
	}
}