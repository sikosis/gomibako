#include "AutoEmptyWarningWindow.h"

#include "Gomibako.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <String.h>
#include <StringView.h>

AutoEmptyWarningWindow::AutoEmptyWarningWindow(const BMessenger& target,
	int32 seconds)
	:
	BWindow(BRect(0.0f, 0.0f, 390.0f, 135.0f), "Scheduled Trash empty",
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE
			| B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(target),
	fNotifyOnClose(true)
{
	BString warning;
	warning.SetToFormat("The Trash will be emptied permanently in %ld seconds.",
		static_cast<long>(seconds));
	BStringView* message = new BStringView("warning", warning.String());
	BButton* skip = new BButton("skip", "Skip this time",
		new BMessage(kMessageSkipAutomaticEmpty));
	skip->SetTarget(target);

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_WINDOW_INSETS)
		.Add(message)
		.Add(new BStringView("explanation",
			"Closing this window also skips the scheduled empty."))
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGlue()
			.Add(skip)
		.End();

	CenterOnScreen();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void AutoEmptyWarningWindow::MessageReceived(BMessage* message) {
	if (message->what == kMessageDismissAutoWarning) {
		fNotifyOnClose = false;
		PostMessage(B_QUIT_REQUESTED);
		return;
	}

	BWindow::MessageReceived(message);
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool AutoEmptyWarningWindow::QuitRequested() {
	if (fNotifyOnClose)
		fTarget.SendMessage(kMessageSkipAutomaticEmpty);
	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------//
