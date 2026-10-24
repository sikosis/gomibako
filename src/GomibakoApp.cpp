#include "GomibakoApp.h"

#include "AboutWindow.h"
#include "AutoEmptyWarningWindow.h"
#include "Gomibako.h"
#include "SettingsWindow.h"

#include <Alert.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Message.h>
#include <MessageRunner.h>
#include <NodeMonitor.h>
#include <OS.h>
#include <Path.h>
#include <PropertyInfo.h>
#include <Roster.h>
#include <String.h>
#include <Volume.h>

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <time.h>

extern char** environ;

static const char* const kVersion = "v0.23";

static property_info kGomibakoProperties[] = {
	{ "Version", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Return the gomibako version", 0, { B_STRING_TYPE }, {}, {} },
	{ "TrashCount", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Return the number of top-level items in the Trash", 0,
		{ B_INT32_TYPE }, {}, {} },
	{ "AutoEmptyMode", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		"Get or set off, startup, daily or interval scheduling", 0,
		{ B_STRING_TYPE }, {}, {} },
	{ "DailyTime", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		"Get or set the daily empty time in HH:MM format", 0,
		{ B_STRING_TYPE }, {}, {} },
	{ "IntervalHours", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		"Get or set the automatic empty interval from 1 to 6 hours", 0,
		{ B_INT32_TYPE }, {}, {} },
	{ "CatchUp", { B_GET_PROPERTY, B_SET_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Get or set whether a missed daily empty runs at launch", 0,
		{ B_BOOL_TYPE }, {}, {} },
	{ "WarnBeforeEmpty", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		"Get or set whether automatic emptying shows a warning", 0,
		{ B_BOOL_TYPE }, {}, {} },
	{ "ConfirmManualEmpty", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		"Get or set whether manual emptying asks for confirmation", 0,
		{ B_BOOL_TYPE }, {}, {} },
	{ "ShowCount", { B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_DIRECT_SPECIFIER },
		"Get or set whether the Deskbar item shows the Trash count", 0,
		{ B_BOOL_TYPE }, {}, {} },
	{ "LastEmptied", { B_GET_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Return the last successful gomibako empty time", 0,
		{ B_INT64_TYPE }, {}, {} },
	{ "OpenTrash", { B_EXECUTE_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Open the Trash in Tracker", 0, {}, {}, {} },
	{ "EmptyTrash", { B_EXECUTE_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Empty the Trash, honouring the manual confirmation setting", 0,
		{}, {}, {} },
	{ "RestoreAll", { B_EXECUTE_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Restore every item in the Trash after confirmation", 0, {}, {}, {} },
	{ "ShowSettings", { B_EXECUTE_PROPERTY }, { B_DIRECT_SPECIFIER },
		"Open the gomibako settings window", 0, {}, {}, {} },
	{ NULL, {}, {}, NULL, 0, {}, {}, {} }
};

static void SendScriptingReply(BMessage* request, status_t error) {
	BMessage reply(B_REPLY);
	reply.AddInt32("error", error);
	request->SendReply(&reply);
}
//---------------------------------------------------------------------------------------------------------------------------------//


static void SendScriptingReply(BMessage* request, status_t error,
	const char* result) {
	BMessage reply(B_REPLY);
	reply.AddString("result", result);
	reply.AddInt32("error", error);
	request->SendReply(&reply);
}
//---------------------------------------------------------------------------------------------------------------------------------//


static void SendScriptingReply(BMessage* request, status_t error,
	int32 result) {
	BMessage reply(B_REPLY);
	reply.AddInt32("result", result);
	reply.AddInt32("error", error);
	request->SendReply(&reply);
}
//---------------------------------------------------------------------------------------------------------------------------------//


static void SendScriptingReply(BMessage* request, status_t error,
	int64 result) {
	BMessage reply(B_REPLY);
	reply.AddInt64("result", result);
	reply.AddInt32("error", error);
	request->SendReply(&reply);
}
//---------------------------------------------------------------------------------------------------------------------------------//


static void SendScriptingReply(BMessage* request, status_t error,
	bool result) {
	BMessage reply(B_REPLY);
	reply.AddBool("result", result);
	reply.AddInt32("error", error);
	request->SendReply(&reply);
}
//---------------------------------------------------------------------------------------------------------------------------------//


static const char* AutoEmptyModeName(int32 mode) {
	switch (mode) {
		case kAutoEmptyStartup:
			return "startup";
		case kAutoEmptyDaily:
			return "daily";
		case kAutoEmptyInterval:
			return "interval";
		default:
			return "off";
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


static bool ParseAutoEmptyMode(const char* value, int32& mode) {
	if (strcasecmp(value, "off") == 0 || strcasecmp(value, "never") == 0) {
		mode = kAutoEmptyOff;
		return true;
	}
	if (strcasecmp(value, "startup") == 0) {
		mode = kAutoEmptyStartup;
		return true;
	}
	if (strcasecmp(value, "daily") == 0) {
		mode = kAutoEmptyDaily;
		return true;
	}
	if (strcasecmp(value, "interval") == 0
		|| strcasecmp(value, "hourly") == 0) {
		mode = kAutoEmptyInterval;
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static bool FindBooleanData(BMessage* message, bool& value) {
	if (message->FindBool("data", &value) == B_OK)
		return true;

	const char* text = NULL;
	if (message->FindString("data", &text) != B_OK)
		return false;
	if (strcasecmp(text, "true") == 0 || strcasecmp(text, "on") == 0
		|| strcmp(text, "1") == 0) {
		value = true;
		return true;
	}
	if (strcasecmp(text, "false") == 0 || strcasecmp(text, "off") == 0
		|| strcmp(text, "0") == 0) {
		value = false;
		return true;
	}
	return false;
}
//---------------------------------------------------------------------------------------------------------------------------------//

struct TrashCommandRequest {
	BMessenger target;
	BString executable;
	std::vector<BString> arguments;
	BString actionName;
	bool recordsEmpty;
	bool scheduled;
};

static int32 RunTrashCommand(void* data) {
	TrashCommandRequest* request = static_cast<TrashCommandRequest*>(data);
	std::vector<const char*> arguments;
	arguments.push_back(request->executable.String());
	for (size_t index = 0; index < request->arguments.size(); index++)
		arguments.push_back(request->arguments[index].String());
	arguments.push_back(NULL);
	std::vector<const char*> environment;
	for (char** variable = environ; *variable != NULL; variable++)
		environment.push_back(*variable);
	environment.push_back(NULL);

	thread_id commandThread = load_image(
		static_cast<int32>(arguments.size() - 1),
		arguments.data(), environment.data());
	status_t result = commandThread;
	if (commandThread >= B_OK) {
		result = resume_thread(commandThread);
		if (result == B_OK) {
			status_t exitStatus = B_ERROR;
			result = wait_for_thread(commandThread, &exitStatus);
			if (result == B_OK)
				result = exitStatus;
		}
	}

	BMessage finished(kMessageCommandFinished);
	finished.AddInt32("status", result);
	finished.AddString("action", request->actionName);
	finished.AddBool("recordsEmpty", request->recordsEmpty);
	finished.AddBool("scheduled", request->scheduled);
	request->target.SendMessage(&finished);
	delete request;
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------------------//

GomibakoApp::GomibakoApp()
	:
	BApplication(kApplicationSignature),
	fDeskbarItemAdded(false),
	fStartupRuleHandled(false),
	fTrashActionRunning(false),
	fTrashCount(0),
	fScheduleTimer(NULL),
	fLaunchTime(time(NULL)),
	fLastScheduleAttempt(0)
{
	fSettings.Load();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::AboutRequested() {
	(new AboutWindow())->Show();
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t GomibakoApp::GetSupportedSuites(BMessage* message) {
	message->AddString("suites", "suite/vnd.sikosis-gomibako");
	BPropertyInfo propertyInfo(kGomibakoProperties);
	message->AddFlat("messages", &propertyInfo);
	return BApplication::GetSupportedSuites(message);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::MessageReceived(BMessage* message) {
	if ((message->what == B_GET_PROPERTY
			|| message->what == B_SET_PROPERTY
			|| message->what == B_EXECUTE_PROPERTY)
		&& _HandleScriptingMessage(message)) {
		return;
	}

	switch (message->what) {
		case kMessageOpenTrash:
			_OpenTrash();
			break;

		case kMessageEmptyTrash:
			if (fTrashCount > 0 && (!fSettings.confirmManualEmpty
				|| _ConfirmAction("Empty Trash",
					"Permanently delete every item in the Trash?",
					"Empty Trash"))) {
				std::vector<BString> arguments;
				arguments.push_back("--empty");
				_LaunchTrashCommand("/bin/trash", arguments, "emptying the Trash",
					true, false);
			}
			break;

		case kMessageRestoreTrash:
			if (fTrashCount > 0 && _ConfirmAction("Restore All",
				"Restore every item to its original location?", "Restore All")) {
				std::vector<BString> arguments;
				arguments.push_back("--all");
				_LaunchTrashCommand("/bin/untrash", arguments,
					"restoring the Trash");
			}
			break;

		case kMessageTrashRefs:
			_TrashRefs(message);
			break;

		case kMessageCommandFinished:
			_HandleCommandFinished(message);
			break;

		case kMessageShowSettings:
			_ShowSettings();
			break;

		case kMessageSettingsChanged:
		{
			const int64 lastEmptied = fSettings.lastEmptied;
			fSettings.SetFromMessage(*message);
			fSettings.lastEmptied = lastEmptied;
			_SettingsChanged();
			break;
		}

		case kMessageScheduleTimer:
			delete fScheduleTimer;
			fScheduleTimer = NULL;
			_BeginScheduledEmpty();
			break;

		case kMessageWarningTimer:
			delete fScheduleTimer;
			fScheduleTimer = NULL;
			_DismissAutoEmptyWarning();
			if (fTrashCount > 0) {
				std::vector<BString> arguments;
				arguments.push_back("--empty");
				_LaunchTrashCommand("/bin/trash", arguments,
					"emptying the Trash automatically", true, true);
			} else {
				_RecordSuccessfulEmpty();
			}
			break;

		case kMessageSkipAutomaticEmpty:
			delete fScheduleTimer;
			fScheduleTimer = NULL;
			_DismissAutoEmptyWarning();
			_ScheduleAutoEmpty();
			break;

		case kMessageRegisterReplicant:
			if (message->FindMessenger("messenger", &fReplicant) == B_OK)
				_SendTrashState();
			break;

		case kMessageQuit:
			_RemoveDeskbarItem();
			PostMessage(B_QUIT_REQUESTED);
			break;

		case B_SOME_APP_LAUNCHED:
		{
			const char* signature = NULL;
			if (message->FindString("be:signature", &signature) == B_OK
				&& strcmp(signature, "application/x-vnd.Be-TSKB") == 0) {
				_AddDeskbarItem();
			}
			break;
		}

		case B_NODE_MONITOR:
		{
			int32 opcode = 0;
			if (message->FindInt32("opcode", &opcode) != B_OK)
				break;

			switch (opcode) {
				case B_DEVICE_MOUNTED:
				case B_DEVICE_UNMOUNTED:
					_RebuildTrashWatches();
					break;

				case B_ENTRY_CREATED:
				case B_ENTRY_REMOVED:
				case B_ENTRY_MOVED:
					_RefreshTrashCount();
					break;
			}
			break;
		}

		default:
			BApplication::MessageReceived(message);
			break;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool GomibakoApp::QuitRequested() {
	delete fScheduleTimer;
	fScheduleTimer = NULL;
	_DismissAutoEmptyWarning();
	_RemoveDeskbarItem();
	fVolumeRoster.StopWatching();
	stop_watching(BMessenger(this));
	be_roster->StopWatching(BMessenger(this));
	return BApplication::QuitRequested();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::ReadyToRun() {
	be_roster->StartWatching(BMessenger(this), B_REQUEST_LAUNCHED);
	fVolumeRoster.StartWatching(BMessenger(this));
	_RebuildTrashWatches();
	_ScheduleAutoEmpty();

	status_t status = _AddDeskbarItem();
	if (status != B_OK && status != B_NO_INIT) {
		fprintf(stderr, "gomibako: could not add the Deskbar item: %s\n",
			strerror(status));
		PostMessage(B_QUIT_REQUESTED);
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


BHandler* GomibakoApp::ResolveSpecifier(BMessage* message, int32 index,
	BMessage* specifier, int32 form, const char* property) {
	BPropertyInfo propertyInfo(kGomibakoProperties);
	if (propertyInfo.FindMatch(message, 0, specifier, form, property) >= 0)
		return this;

	return BApplication::ResolveSpecifier(message, index, specifier, form,
		property);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_BeginScheduledEmpty() {
	if (fSettings.mode == kAutoEmptyOff)
		return;

	fLastScheduleAttempt = time(NULL);
	if (fSettings.mode == kAutoEmptyStartup)
		fStartupRuleHandled = true;
	if (fTrashActionRunning) {
		_ScheduleAutoEmpty();
		return;
	}

	if (fTrashCount == 0) {
		_RecordSuccessfulEmpty();
		return;
	}

	if (!fSettings.warnBeforeEmpty) {
		std::vector<BString> arguments;
		arguments.push_back("--empty");
		_LaunchTrashCommand("/bin/trash", arguments,
			"emptying the Trash automatically", true, true);
		return;
	}

	AutoEmptyWarningWindow* warning = new AutoEmptyWarningWindow(
		BMessenger(this), fSettings.warnSeconds);
	fWarningWindow = BMessenger(warning);
	warning->Show();
	BMessage timerMessage(kMessageWarningTimer);
	fScheduleTimer = new BMessageRunner(BMessenger(this), &timerMessage,
		static_cast<bigtime_t>(fSettings.warnSeconds) * 1000000, 1);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_DismissAutoEmptyWarning() {
	if (fWarningWindow.IsValid())
		fWarningWindow.SendMessage(kMessageDismissAutoWarning);
	fWarningWindow = BMessenger();
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t GomibakoApp::_AddDeskbarItem() {
	BDeskbar deskbar;
	if (!deskbar.IsRunning())
		return B_NO_INIT;

	if (deskbar.HasItem(kDeskbarItemName)) {
		fDeskbarItemAdded = true;
		return B_OK;
	}

	app_info appInfo;
	status_t status = GetAppInfo(&appInfo);
	if (status != B_OK)
		return status;

	status = deskbar.AddItem(&appInfo.ref);
	if (status == B_OK)
		fDeskbarItemAdded = true;

	return status;
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool GomibakoApp::_ConfirmAction(const char* title, const char* text,
	const char* actionLabel) {
	BAlert* alert = new BAlert(title, text, "Cancel", actionLabel, NULL,
		B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	return alert->Go() == 1;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_HandleCommandFinished(BMessage* message) {
	fTrashActionRunning = false;
	_RefreshTrashCount();

	int32 status = B_ERROR;
	message->FindInt32("status", &status);
	bool recordsEmpty = false;
	message->FindBool("recordsEmpty", &recordsEmpty);
	bool scheduled = false;
	message->FindBool("scheduled", &scheduled);
	if (status == B_OK) {
		if (recordsEmpty)
			_RecordSuccessfulEmpty();
		return;
	}

	if (scheduled)
		_ScheduleAutoEmpty();

	const char* action = "performing the Trash action";
	message->FindString("action", &action);
	BString errorText;
	if (status > B_OK) {
		errorText.SetToFormat("An error occurred while %s.\n\n"
			"The command exited with status %ld.", action,
			static_cast<long>(status));
	} else {
		errorText.SetToFormat("An error occurred while %s.\n\n%s", action,
			strerror(status));
	}
	BAlert* alert = new BAlert("Trash action failed", errorText.String(), "OK",
		NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go(NULL);
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool GomibakoApp::_HandleScriptingMessage(BMessage* message) {
	int32 specifierIndex = 0;
	BMessage specifier;
	BString property;
	if (message->GetCurrentSpecifier(&specifierIndex, &specifier) != B_OK
		|| specifier.FindString("property", &property) != B_OK) {
		return false;
	}

	if (message->what == B_GET_PROPERTY) {
		if (property == "Version") {
			SendScriptingReply(message, B_OK, kVersion);
			return true;
		}
		if (property == "TrashCount") {
			SendScriptingReply(message, B_OK, fTrashCount);
			return true;
		}
		if (property == "AutoEmptyMode") {
			SendScriptingReply(message, B_OK,
				AutoEmptyModeName(fSettings.mode));
			return true;
		}
		if (property == "DailyTime") {
			BString time;
			time.SetToFormat("%02ld:%02ld",
				static_cast<long>(fSettings.dailyMinutes / 60),
				static_cast<long>(fSettings.dailyMinutes % 60));
			SendScriptingReply(message, B_OK, time.String());
			return true;
		}
		if (property == "IntervalHours") {
			SendScriptingReply(message, B_OK, fSettings.intervalHours);
			return true;
		}
		if (property == "CatchUp") {
			SendScriptingReply(message, B_OK, fSettings.catchUp);
			return true;
		}
		if (property == "WarnBeforeEmpty") {
			SendScriptingReply(message, B_OK, fSettings.warnBeforeEmpty);
			return true;
		}
		if (property == "ConfirmManualEmpty") {
			SendScriptingReply(message, B_OK,
				fSettings.confirmManualEmpty);
			return true;
		}
		if (property == "ShowCount") {
			SendScriptingReply(message, B_OK, fSettings.showCount);
			return true;
		}
		if (property == "LastEmptied") {
			SendScriptingReply(message, B_OK, fSettings.lastEmptied);
			return true;
		}
	}

	if (message->what == B_SET_PROPERTY) {
		if (property == "AutoEmptyMode") {
			const char* value = NULL;
			int32 mode = kAutoEmptyOff;
			if (message->FindString("data", &value) != B_OK
				|| !ParseAutoEmptyMode(value, mode)) {
				SendScriptingReply(message, B_BAD_VALUE);
				return true;
			}
			fSettings.mode = mode;
			_SettingsChanged();
			SendScriptingReply(message, B_OK, AutoEmptyModeName(mode));
			return true;
		}

		if (property == "DailyTime") {
			const char* value = NULL;
			int hours = 0;
			int minutes = 0;
			char trailing = '\0';
			if (message->FindString("data", &value) != B_OK
				|| sscanf(value, "%d:%d%c", &hours, &minutes, &trailing) != 2
				|| hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
				SendScriptingReply(message, B_BAD_VALUE);
				return true;
			}
			fSettings.dailyMinutes = hours * 60 + minutes;
			_SettingsChanged();
			BString result;
			result.SetToFormat("%02d:%02d", hours, minutes);
			SendScriptingReply(message, B_OK, result.String());
			return true;
		}

		if (property == "IntervalHours") {
			int32 hours = 0;
			if (message->FindInt32("data", &hours) != B_OK) {
				const char* value = NULL;
				int parsedHours = 0;
				char trailing = '\0';
				if (message->FindString("data", &value) != B_OK
					|| sscanf(value, "%d%c", &parsedHours, &trailing) != 1) {
					SendScriptingReply(message, B_BAD_VALUE);
					return true;
				}
				hours = parsedHours;
			}
			if (hours < 1 || hours > 6) {
				SendScriptingReply(message, B_BAD_VALUE);
				return true;
			}
			fSettings.intervalHours = hours;
			_SettingsChanged();
			SendScriptingReply(message, B_OK, hours);
			return true;
		}

		bool value = false;
		if (property == "CatchUp" || property == "WarnBeforeEmpty"
			|| property == "ConfirmManualEmpty" || property == "ShowCount") {
			if (!FindBooleanData(message, value)) {
				SendScriptingReply(message, B_BAD_VALUE);
				return true;
			}
			if (property == "CatchUp")
				fSettings.catchUp = value;
			else if (property == "WarnBeforeEmpty")
				fSettings.warnBeforeEmpty = value;
			else if (property == "ConfirmManualEmpty")
				fSettings.confirmManualEmpty = value;
			else
				fSettings.showCount = value;
			_SettingsChanged();
			SendScriptingReply(message, B_OK, value);
			return true;
		}
	}

	if (message->what == B_EXECUTE_PROPERTY) {
		uint32 command = 0;
		if (property == "OpenTrash")
			command = kMessageOpenTrash;
		else if (property == "EmptyTrash")
			command = kMessageEmptyTrash;
		else if (property == "RestoreAll")
			command = kMessageRestoreTrash;
		else if (property == "ShowSettings")
			command = kMessageShowSettings;
		else
			return false;

		SendScriptingReply(message, PostMessage(command));
		return true;
	}

	return false;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_LaunchTrashCommand(const char* executable,
	const std::vector<BString>& arguments, const char* actionName,
	bool recordsEmpty, bool scheduled) {
	if (fTrashActionRunning) {
		if (scheduled)
			_ScheduleAutoEmpty();
		return;
	}

	TrashCommandRequest* request = new TrashCommandRequest;
	request->target = BMessenger(this);
	request->executable = executable;
	request->arguments = arguments;
	request->actionName = actionName;
	request->recordsEmpty = recordsEmpty;
	request->scheduled = scheduled;

	thread_id worker = spawn_thread(RunTrashCommand, "gomibako Trash action",
		B_NORMAL_PRIORITY, request);
	if (worker < B_OK) {
		delete request;
		BString errorText;
		errorText.SetToFormat("Could not start the Trash action.\n\n%s",
			strerror(worker));
		(new BAlert("Trash action failed", errorText.String(), "OK"))->Go(NULL);
		if (scheduled)
			_ScheduleAutoEmpty();
		return;
	}

	status_t status = resume_thread(worker);
	if (status != B_OK) {
		kill_thread(worker);
		delete request;
		BString errorText;
		errorText.SetToFormat("Could not start the Trash action.\n\n%s",
			strerror(status));
		(new BAlert("Trash action failed", errorText.String(), "OK"))->Go(NULL);
		if (scheduled)
			_ScheduleAutoEmpty();
		return;
	}

	fTrashActionRunning = true;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_RecordSuccessfulEmpty() {
	_DismissAutoEmptyWarning();
	fSettings.lastEmptied = time(NULL);
	fLastScheduleAttempt = fSettings.lastEmptied;
	fSettings.Save();
	_ScheduleAutoEmpty();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_OpenTrash() {
	BPath trashPath;
	if (find_directory(B_TRASH_DIRECTORY, &trashPath) != B_OK)
		return;

	BEntry trashEntry(trashPath.Path(), true);
	entry_ref trashRef;
	if (trashEntry.GetRef(&trashRef) != B_OK)
		return;

	BMessage refs(B_REFS_RECEIVED);
	refs.AddRef("refs", &trashRef);
	be_roster->Launch("application/x-vnd.Be-TRAK", &refs);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_RebuildTrashWatches() {
	BMessenger target(this);
	for (size_t index = 0; index < fWatchedTrashDirectories.size(); index++) {
		watch_node(&fWatchedTrashDirectories[index], B_STOP_WATCHING, target);
	}
	fWatchedTrashDirectories.clear();

	fVolumeRoster.Rewind();
	BVolume volume;
	while (fVolumeRoster.GetNextVolume(&volume) == B_OK) {
		if (volume.IsReadOnly())
			continue;

		BPath trashPath;
		if (find_directory(B_TRASH_DIRECTORY, &trashPath, true, &volume)
			!= B_OK) {
			continue;
		}

		BDirectory trashDirectory(trashPath.Path());
		if (trashDirectory.InitCheck() != B_OK)
			continue;

		node_ref nodeRef;
		if (trashDirectory.GetNodeRef(&nodeRef) != B_OK)
			continue;

		if (watch_node(&nodeRef, B_WATCH_DIRECTORY, target) == B_OK)
			fWatchedTrashDirectories.push_back(nodeRef);
	}

	_RefreshTrashCount();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_TrashRefs(BMessage* message) {
	std::vector<BString> arguments;
	entry_ref ref;
	for (int32 index = 0; message->FindRef("refs", index, &ref) == B_OK;
		index++) {
		BEntry entry(&ref, true);
		BPath path;
		if (entry.GetPath(&path) == B_OK)
			arguments.push_back(path.Path());
	}

	if (!arguments.empty())
		_LaunchTrashCommand("/bin/trash", arguments, "moving items to the Trash");
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_RefreshTrashCount() {
	int32 total = 0;
	fVolumeRoster.Rewind();
	BVolume volume;
	while (fVolumeRoster.GetNextVolume(&volume) == B_OK) {
		if (volume.IsReadOnly())
			continue;

		BPath trashPath;
		if (find_directory(B_TRASH_DIRECTORY, &trashPath, false, &volume)
			!= B_OK) {
			continue;
		}

		BDirectory trashDirectory(trashPath.Path());
		if (trashDirectory.InitCheck() != B_OK)
			continue;

		int32 count = trashDirectory.CountEntries();
		if (count > 0)
			total += count;
	}

	if (total == fTrashCount)
		return;

	fTrashCount = total;
	_SendTrashState();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_RemoveDeskbarItem() {
	if (!fDeskbarItemAdded)
		return;

	BDeskbar deskbar;
	if (deskbar.IsRunning())
		deskbar.RemoveItem(kDeskbarItemName);

	fDeskbarItemAdded = false;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_ScheduleAutoEmpty() {
	delete fScheduleTimer;
	fScheduleTimer = NULL;
	if (fSettings.mode == kAutoEmptyOff)
		return;

	const time_t now = time(NULL);
	time_t due = now + 1;
	switch (fSettings.mode) {
		case kAutoEmptyStartup:
			if (fStartupRuleHandled)
				return;
			due = fLaunchTime + 60;
			break;

		case kAutoEmptyDaily:
		{
			struct tm localNow = *localtime(&now);
			localNow.tm_hour = fSettings.dailyMinutes / 60;
			localNow.tm_min = fSettings.dailyMinutes % 60;
			localNow.tm_sec = 0;
			time_t todayDue = mktime(&localNow);
			struct tm nextLocal = localNow;
			nextLocal.tm_mday++;
			time_t nextDue = mktime(&nextLocal);
			const bool attemptedToday = fLastScheduleAttempt >= todayDue
				&& fLastScheduleAttempt < nextDue;
			if (now < todayDue) {
				due = todayDue;
			} else if (fSettings.catchUp
				&& fSettings.lastEmptied < todayDue && !attemptedToday) {
				due = now + 1;
			} else {
				due = nextDue;
			}
			break;
		}

		case kAutoEmptyInterval:
		{
			time_t base = static_cast<time_t>(fSettings.lastEmptied);
			if (base <= 0 || base > now)
				base = now;
			if (fLastScheduleAttempt > base && fLastScheduleAttempt <= now)
				base = fLastScheduleAttempt;
			due = base + fSettings.intervalHours * 60 * 60;
			if (due <= now)
				due = now + 1;
			break;
		}

		default:
			return;
	}

	if (due <= now)
		due = now + 1;
	BMessage timerMessage(kMessageScheduleTimer);
	fScheduleTimer = new BMessageRunner(BMessenger(this), &timerMessage,
		static_cast<bigtime_t>(due - now) * 1000000, 1);
}
//---------------------------------------------------------------------------------------------------------------------------------//


BString GomibakoApp::_ScheduleDescription() const {
	BString description("Auto-empty: Off");
	switch (fSettings.mode) {
		case kAutoEmptyStartup:
			description = "Auto-empty: On startup";
			break;

		case kAutoEmptyDaily:
			description.SetToFormat("Auto-empty: Daily at %02ld:%02ld",
				static_cast<long>(fSettings.dailyMinutes / 60),
				static_cast<long>(fSettings.dailyMinutes % 60));
			break;

		case kAutoEmptyInterval:
			if (fSettings.intervalHours == 1) {
				description = "Auto-empty: Every hour";
			} else {
				description.SetToFormat("Auto-empty: Every %ld hours",
					static_cast<long>(fSettings.intervalHours));
			}
			break;

		default:
			break;
	}
	return description;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_SendTrashState() {
	if (!fReplicant.IsValid())
		return;

	BMessage state(kMessageTrashState);
	state.AddInt32("count", fTrashCount);
	state.AddBool("showCount", fSettings.showCount);
	state.AddString("schedule", _ScheduleDescription());
	if (fReplicant.SendMessage(&state) != B_OK)
		fReplicant = BMessenger();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_SettingsChanged() {
	fSettings.Save();
	_DismissAutoEmptyWarning();
	_ScheduleAutoEmpty();
	_SendTrashState();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::_ShowSettings() {
	if (fSettingsWindow.IsValid()
		&& fSettingsWindow.SendMessage(kMessageActivateSettings) == B_OK) {
		return;
	}

	SettingsWindow* window = new SettingsWindow(fSettings, BMessenger(this));
	fSettingsWindow = BMessenger(window);
	window->Show();
}
//---------------------------------------------------------------------------------------------------------------------------------//
