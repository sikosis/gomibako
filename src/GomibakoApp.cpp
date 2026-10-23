#include "GomibakoApp.h"

#include "AboutWindow.h"
#include "Gomibako.h"

#include <Alert.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Message.h>
#include <NodeMonitor.h>
#include <OS.h>
#include <Path.h>
#include <Roster.h>
#include <String.h>
#include <Volume.h>

#include <stdio.h>
#include <string.h>

extern char** environ;

struct TrashCommandRequest {
	BMessenger target;
	BString executable;
	std::vector<BString> arguments;
	BString actionName;
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
	request->target.SendMessage(&finished);
	delete request;
	return 0;
}
//---------------------------------------------------------------------------------------------------------------------------------//

GomibakoApp::GomibakoApp()
	:
	BApplication(kApplicationSignature),
	fDeskbarItemAdded(false),
	fTrashActionRunning(false),
	fTrashCount(0)
{
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::AboutRequested() {
	(new AboutWindow())->Show();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::MessageReceived(BMessage* message) {
	switch (message->what) {
		case kMessageOpenTrash:
			_OpenTrash();
			break;

		case kMessageEmptyTrash:
			if (fTrashCount > 0 && _ConfirmAction("Empty Trash",
				"Permanently delete every item in the Trash?",
				"Empty Trash")) {
				std::vector<BString> arguments;
				arguments.push_back("--empty");
				_LaunchTrashCommand("/bin/trash", arguments, "emptying the Trash");
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

		case kMessageRegisterReplicant:
			if (message->FindMessenger("messenger", &fReplicant) == B_OK)
				_SendTrashState();
			break;

		case kMessageRemoveFromDeskbar:
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

	status_t status = _AddDeskbarItem();
	if (status != B_OK && status != B_NO_INIT) {
		fprintf(stderr, "gomibako: could not add the Deskbar item: %s\n",
			strerror(status));
		PostMessage(B_QUIT_REQUESTED);
	}
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
	if (status == B_OK)
		return;

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


void GomibakoApp::_LaunchTrashCommand(const char* executable,
	const std::vector<BString>& arguments, const char* actionName) {
	if (fTrashActionRunning)
		return;

	TrashCommandRequest* request = new TrashCommandRequest;
	request->target = BMessenger(this);
	request->executable = executable;
	request->arguments = arguments;
	request->actionName = actionName;

	thread_id worker = spawn_thread(RunTrashCommand, "gomibako Trash action",
		B_NORMAL_PRIORITY, request);
	if (worker < B_OK) {
		delete request;
		BString errorText;
		errorText.SetToFormat("Could not start the Trash action.\n\n%s",
			strerror(worker));
		(new BAlert("Trash action failed", errorText.String(), "OK"))->Go(NULL);
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
		return;
	}

	fTrashActionRunning = true;
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


void GomibakoApp::_SendTrashState() {
	if (!fReplicant.IsValid())
		return;

	BMessage state(kMessageTrashState);
	state.AddInt32("count", fTrashCount);
	if (fReplicant.SendMessage(&state) != B_OK)
		fReplicant = BMessenger();
}
//---------------------------------------------------------------------------------------------------------------------------------//
