#include "Settings.h"

#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>

#include <algorithm>

static const char* const kSettingsFileName = "DeskTrash_settings";

GomibakoSettings::GomibakoSettings() {
	SetDefaults();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoSettings::AddToMessage(BMessage& message) const {
	message.AddInt32("mode", mode);
	message.AddInt32("dailyMinutes", dailyMinutes);
	message.AddInt32("intervalHours", intervalHours);
	message.AddBool("catchUp", catchUp);
	message.AddBool("warnBeforeEmpty", warnBeforeEmpty);
	message.AddInt32("warnSeconds", warnSeconds);
	message.AddBool("confirmManualEmpty", confirmManualEmpty);
	message.AddBool("showCount", showCount);
	message.AddString("iconTheme", iconTheme);
	message.AddInt64("lastEmptied", lastEmptied);
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t GomibakoSettings::Load() {
	SetDefaults();
	BPath settingsPath;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath);
	if (status != B_OK)
		return status;
	status = settingsPath.Append(kSettingsFileName);
	if (status != B_OK)
		return status;

	BFile file(settingsPath.Path(), B_READ_ONLY);
	status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage message;
	status = message.Unflatten(&file);
	if (status == B_OK)
		SetFromMessage(message);
	return status;
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t GomibakoSettings::Save() const {
	BPath settingsPath;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &settingsPath,
		true);
	if (status != B_OK)
		return status;
	status = settingsPath.Append(kSettingsFileName);
	if (status != B_OK)
		return status;

	BFile file(settingsPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status = file.InitCheck();
	if (status != B_OK)
		return status;

	BMessage message;
	AddToMessage(message);
	return message.Flatten(&file);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoSettings::SetDefaults() {
	mode = kAutoEmptyOff;
	dailyMinutes = 18 * 60;
	intervalHours = 3;
	catchUp = true;
	warnBeforeEmpty = true;
	warnSeconds = 30;
	confirmManualEmpty = true;
	showCount = true;
	iconTheme = "system";
	lastEmptied = 0;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoSettings::SetFromMessage(const BMessage& message) {
	int32 intValue = 0;
	bool boolValue = false;
	const char* stringValue = NULL;
	int64 int64Value = 0;

	if (message.FindInt32("mode", &intValue) == B_OK)
		mode = std::max(static_cast<int32>(kAutoEmptyOff),
			std::min(static_cast<int32>(kAutoEmptyInterval), intValue));
	if (message.FindInt32("dailyMinutes", &intValue) == B_OK)
		dailyMinutes = std::max(0, std::min(1439, intValue));
	if (message.FindInt32("intervalHours", &intValue) == B_OK)
		intervalHours = std::max(1, std::min(6, intValue));
	if (message.FindBool("catchUp", &boolValue) == B_OK)
		catchUp = boolValue;
	if (message.FindBool("warnBeforeEmpty", &boolValue) == B_OK)
		warnBeforeEmpty = boolValue;
	if (message.FindInt32("warnSeconds", &intValue) == B_OK)
		warnSeconds = std::max(5, std::min(300, intValue));
	if (message.FindBool("confirmManualEmpty", &boolValue) == B_OK)
		confirmManualEmpty = boolValue;
	if (message.FindBool("showCount", &boolValue) == B_OK)
		showCount = boolValue;
	if (message.FindString("iconTheme", &stringValue) == B_OK)
		iconTheme = stringValue;
	if (message.FindInt64("lastEmptied", &int64Value) == B_OK)
		lastEmptied = std::max(static_cast<int64>(0), int64Value);
}
//---------------------------------------------------------------------------------------------------------------------------------//
