#include "GomibakoApp.h"

#include "AboutWindow.h"
#include "Gomibako.h"

#include <Deskbar.h>
#include <Message.h>
#include <Roster.h>

#include <stdio.h>
#include <string.h>

GomibakoApp::GomibakoApp()
	:
	BApplication(kApplicationSignature),
	fDeskbarItemAdded(false)
{
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::AboutRequested() {
	(new AboutWindow())->Show();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::MessageReceived(BMessage* message) {
	switch (message->what) {
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

		default:
			BApplication::MessageReceived(message);
			break;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool GomibakoApp::QuitRequested() {
	_RemoveDeskbarItem();
	be_roster->StopWatching(BMessenger(this));
	return BApplication::QuitRequested();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void GomibakoApp::ReadyToRun() {
	be_roster->StartWatching(BMessenger(this), B_REQUEST_LAUNCHED);

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


void GomibakoApp::_RemoveDeskbarItem() {
	if (!fDeskbarItemAdded)
		return;

	BDeskbar deskbar;
	if (deskbar.IsRunning())
		deskbar.RemoveItem(kDeskbarItemName);

	fDeskbarItemAdded = false;
}
//---------------------------------------------------------------------------------------------------------------------------------//
