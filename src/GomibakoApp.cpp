#include "GomibakoApp.h"

#include "AboutWindow.h"
#include "Gomibako.h"

#include <Deskbar.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Message.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Roster.h>
#include <Volume.h>

#include <stdio.h>
#include <string.h>

GomibakoApp::GomibakoApp()
	:
	BApplication(kApplicationSignature),
	fDeskbarItemAdded(false),
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
