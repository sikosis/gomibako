#ifndef SETTINGS_H
#define SETTINGS_H

#include <String.h>
#include <SupportDefs.h>

class BMessage;

enum AutoEmptyMode {
	kAutoEmptyOff = 0,
	kAutoEmptyStartup,
	kAutoEmptyDaily,
	kAutoEmptyInterval
};

struct GomibakoSettings {
	GomibakoSettings();

	void AddToMessage(BMessage& message) const;
	status_t Load();
	status_t Save() const;
	void SetDefaults();
	void SetFromMessage(const BMessage& message);

	int32 mode;
	int32 dailyMinutes;
	int32 intervalHours;
	bool catchUp;
	bool warnBeforeEmpty;
	int32 warnSeconds;
	bool confirmManualEmpty;
	bool showCount;
	BString iconTheme;
	int64 lastEmptied;
};

#endif
