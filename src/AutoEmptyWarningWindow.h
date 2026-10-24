#ifndef AUTO_EMPTY_WARNING_WINDOW_H
#define AUTO_EMPTY_WARNING_WINDOW_H

#include <Messenger.h>
#include <Window.h>

class AutoEmptyWarningWindow : public BWindow {
public:
	AutoEmptyWarningWindow(const BMessenger& target, int32 seconds);

	void MessageReceived(BMessage* message) override;
	bool QuitRequested() override;

private:
	BMessenger fTarget;
	bool fNotifyOnClose;
};

#endif
