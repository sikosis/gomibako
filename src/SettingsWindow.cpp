#include "SettingsWindow.h"

#include "Gomibako.h"

#include <Button.h>
#include <CheckBox.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <SeparatorView.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>

#include <stdio.h>

static BMenuItem* AddOption(BPopUpMenu* menu, const char* label,
	const char* field, int32 value) {
	BMessage* message = new BMessage(kMessageSettingsControl);
	message->AddInt32(field, value);
	BMenuItem* item = new BMenuItem(label, message);
	menu->AddItem(item);
	return item;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static int32 MarkedValue(BPopUpMenu* menu, const char* field,
	int32 fallback) {
	BMenuItem* item = menu->FindMarked();
	if (item == NULL || item->Message() == NULL)
		return fallback;
	return item->Message()->GetInt32(field, fallback);
}
//---------------------------------------------------------------------------------------------------------------------------------//


static void MarkValue(BPopUpMenu* menu, const char* field, int32 value) {
	for (int32 index = 0; index < menu->CountItems(); index++) {
		BMenuItem* item = menu->ItemAt(index);
		if (item != NULL && item->Message() != NULL
			&& item->Message()->GetInt32(field, -1) == value) {
			item->SetMarked(true);
			return;
		}
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


SettingsWindow::SettingsWindow(const GomibakoSettings& settings,
	const BMessenger& target)
	:
	BWindow(BRect(0.0f, 0.0f, 510.0f, 390.0f), "gomibako settings",
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE
			| B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(target),
	fInitialSettings(settings),
	fModeMenu(new BPopUpMenu("automatic empty mode")),
	fIntervalMenu(new BPopUpMenu("automatic empty interval")),
	fDailyTime(new BTextControl("daily time", "Time (HH:MM):", "18:00",
		new BMessage(kMessageSettingsControl))),
	fCatchUp(new BCheckBox("catch up",
		"Run a missed daily empty when gomibako starts",
		new BMessage(kMessageSettingsControl))),
	fWarnBeforeEmpty(new BCheckBox("warn before empty",
		"Warn me 30 seconds before an automatic empty",
		new BMessage(kMessageSettingsControl))),
	fConfirmManualEmpty(new BCheckBox("confirm manual empty",
		"Ask before Empty Trash from the menu",
		new BMessage(kMessageSettingsControl))),
	fShowCount(new BCheckBox("show item count",
		"Show the item count next to the Deskbar icon",
		new BMessage(kMessageSettingsControl)))
{
	AddOption(fModeMenu, "Never", "mode", kAutoEmptyOff);
	AddOption(fModeMenu, "When gomibako starts", "mode", kAutoEmptyStartup);
	AddOption(fModeMenu, "Every day at", "mode", kAutoEmptyDaily);
	AddOption(fModeMenu, "Every N hours", "mode", kAutoEmptyInterval);
	for (int32 hours = 1; hours <= 6; hours++) {
		BString label;
		label.SetToFormat(hours == 1 ? "%ld hour" : "%ld hours",
			static_cast<long>(hours));
		AddOption(fIntervalMenu, label.String(), "intervalHours", hours);
	}

	BMenuField* modeField = new BMenuField("mode field",
		"Empty the Trash automatically:", fModeMenu);
	BMenuField* intervalField = new BMenuField("interval field",
		"Interval:", fIntervalMenu);
	BStringView* warning = new BStringView("permanent warning",
		"Automatic emptying permanently deletes everything in the Trash.");
	warning->SetHighUIColor(B_FAILURE_COLOR);

	BButton* emptyNow = new BButton("empty now", "Empty Trash Now" B_UTF8_ELLIPSIS,
		new BMessage(kMessageEmptyTrash));
	emptyNow->SetTarget(target);
	BButton* defaults = new BButton("defaults", "Defaults",
		new BMessage(kMessageSettingsDefaults));
	BButton* revert = new BButton("revert", "Revert",
		new BMessage(kMessageSettingsRevert));
	BButton* close = new BButton("close", "Close",
		new BMessage(B_QUIT_REQUESTED));

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(warning)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(modeField)
		.Add(fDailyTime)
		.Add(intervalField)
		.Add(fCatchUp)
		.Add(fWarnBeforeEmpty)
		.Add(fConfirmManualEmpty)
		.Add(fShowCount)
		.AddGlue()
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(emptyNow)
			.AddGlue()
			.Add(defaults)
			.Add(revert)
			.Add(close)
		.End();

	fModeMenu->SetTargetForItems(this);
	fIntervalMenu->SetTargetForItems(this);
	fDailyTime->SetModificationMessage(new BMessage(kMessageSettingsControl));
	_ApplySettings(settings);
	CenterOnScreen();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void SettingsWindow::MessageReceived(BMessage* message) {
	switch (message->what) {
		case kMessageSettingsControl:
			_UpdateEnabledControls();
			_SendSettings();
			break;

		case kMessageSettingsDefaults:
		{
			GomibakoSettings defaults;
			_ApplySettings(defaults);
			_SendSettings();
			break;
		}

		case kMessageSettingsRevert:
			_ApplySettings(fInitialSettings);
			_SendSettings();
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


void SettingsWindow::_ApplySettings(const GomibakoSettings& settings) {
	MarkValue(fModeMenu, "mode", settings.mode);
	MarkValue(fIntervalMenu, "intervalHours", settings.intervalHours);
	BString time;
	time.SetToFormat("%02ld:%02ld", static_cast<long>(settings.dailyMinutes / 60),
		static_cast<long>(settings.dailyMinutes % 60));
	fDailyTime->SetText(time.String());
	fCatchUp->SetValue(settings.catchUp ? B_CONTROL_ON : B_CONTROL_OFF);
	fWarnBeforeEmpty->SetValue(settings.warnBeforeEmpty
		? B_CONTROL_ON : B_CONTROL_OFF);
	fConfirmManualEmpty->SetValue(settings.confirmManualEmpty
		? B_CONTROL_ON : B_CONTROL_OFF);
	fShowCount->SetValue(settings.showCount ? B_CONTROL_ON : B_CONTROL_OFF);
	_UpdateEnabledControls();
}
//---------------------------------------------------------------------------------------------------------------------------------//


GomibakoSettings SettingsWindow::_ReadSettings() const {
	GomibakoSettings settings = fInitialSettings;
	settings.mode = MarkedValue(fModeMenu, "mode", kAutoEmptyOff);
	settings.intervalHours = MarkedValue(fIntervalMenu, "intervalHours", 3);
	int hours = 0;
	int minutes = 0;
	if (sscanf(fDailyTime->Text(), "%d:%d", &hours, &minutes) == 2
		&& hours >= 0 && hours <= 23 && minutes >= 0 && minutes <= 59) {
		settings.dailyMinutes = hours * 60 + minutes;
	}
	settings.catchUp = fCatchUp->Value() == B_CONTROL_ON;
	settings.warnBeforeEmpty = fWarnBeforeEmpty->Value() == B_CONTROL_ON;
	settings.confirmManualEmpty = fConfirmManualEmpty->Value() == B_CONTROL_ON;
	settings.showCount = fShowCount->Value() == B_CONTROL_ON;
	return settings;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void SettingsWindow::_SendSettings() {
	BMessage settingsMessage(kMessageSettingsChanged);
	_ReadSettings().AddToMessage(settingsMessage);
	fTarget.SendMessage(&settingsMessage);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void SettingsWindow::_UpdateEnabledControls() {
	const int32 mode = MarkedValue(fModeMenu, "mode", kAutoEmptyOff);
	fDailyTime->SetEnabled(mode == kAutoEmptyDaily);
	fIntervalMenu->SetEnabled(mode == kAutoEmptyInterval);
	fCatchUp->SetEnabled(mode == kAutoEmptyDaily);
}
//---------------------------------------------------------------------------------------------------------------------------------//
