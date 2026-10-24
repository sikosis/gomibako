#ifndef GOMIBAKO_APP_H
#define GOMIBAKO_APP_H

#include <Application.h>
#include <Messenger.h>
#include <Node.h>
#include <String.h>
#include <VolumeRoster.h>

#include <vector>
#include <time.h>

#include "Settings.h"

class BMessageRunner;

class GomibakoApp : public BApplication {
public:
	GomibakoApp();

	void AboutRequested() override;
	void MessageReceived(BMessage* message) override;
	bool QuitRequested() override;
	void ReadyToRun() override;

private:
	status_t _AddDeskbarItem();
	bool _ConfirmAction(const char* title, const char* text,
		const char* actionLabel);
	void _BeginScheduledEmpty();
	void _DismissAutoEmptyWarning();
	void _HandleCommandFinished(BMessage* message);
	void _LaunchTrashCommand(const char* executable,
		const std::vector<BString>& arguments, const char* actionName,
		bool recordsEmpty = false, bool scheduled = false);
	void _OpenTrash();
	void _RecordSuccessfulEmpty();
	void _RebuildTrashWatches();
	void _RefreshTrashCount();
	void _RemoveDeskbarItem();
	void _ScheduleAutoEmpty();
	BString _ScheduleDescription() const;
	void _SendTrashState();
	void _ShowSettings();
	void _TrashRefs(BMessage* message);

	bool fDeskbarItemAdded;
	bool fStartupRuleHandled;
	bool fTrashActionRunning;
	BMessenger fSettingsWindow;
	BMessenger fWarningWindow;
	BMessenger fReplicant;
	int32 fTrashCount;
	BMessageRunner* fScheduleTimer;
	GomibakoSettings fSettings;
	time_t fLaunchTime;
	time_t fLastScheduleAttempt;
	std::vector<node_ref> fWatchedTrashDirectories;
	BVolumeRoster fVolumeRoster;
};

#endif
