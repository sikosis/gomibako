#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include "Settings.h"

#include <Messenger.h>
#include <Window.h>

class BCheckBox;
class BMenuItem;
class BPopUpMenu;
class BTextControl;
class BView;

class SettingsWindow : public BWindow {
public:
	SettingsWindow(const GomibakoSettings& settings,
		const BMessenger& target);

	void MessageReceived(BMessage* message) override;

private:
	void _ApplySettings(const GomibakoSettings& settings);
	GomibakoSettings _ReadSettings() const;
	void _SendSettings();
	void _UpdateEnabledControls();

	BMessenger fTarget;
	GomibakoSettings fInitialSettings;
	BPopUpMenu* fModeMenu;
	BPopUpMenu* fIntervalMenu;
	BTextControl* fDailyTime;
	BCheckBox* fCatchUp;
	BCheckBox* fWarnBeforeEmpty;
	BCheckBox* fConfirmManualEmpty;
	BCheckBox* fShowCount;
};

#endif
