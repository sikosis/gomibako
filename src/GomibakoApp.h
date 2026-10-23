#ifndef GOMIBAKO_APP_H
#define GOMIBAKO_APP_H

#include <Application.h>

class GomibakoApp : public BApplication {
public:
	GomibakoApp();

	void AboutRequested() override;
	void MessageReceived(BMessage* message) override;
	bool QuitRequested() override;
	void ReadyToRun() override;

private:
	status_t _AddDeskbarItem();
	void _RemoveDeskbarItem();

	bool fDeskbarItemAdded;
};

#endif
