#ifndef GOMIBAKO_APP_H
#define GOMIBAKO_APP_H

#include <Application.h>
#include <Messenger.h>
#include <Node.h>
#include <VolumeRoster.h>

#include <vector>

class GomibakoApp : public BApplication {
public:
	GomibakoApp();

	void AboutRequested() override;
	void MessageReceived(BMessage* message) override;
	bool QuitRequested() override;
	void ReadyToRun() override;

private:
	status_t _AddDeskbarItem();
	void _RebuildTrashWatches();
	void _RefreshTrashCount();
	void _RemoveDeskbarItem();
	void _SendTrashState();

	bool fDeskbarItemAdded;
	BMessenger fReplicant;
	int32 fTrashCount;
	std::vector<node_ref> fWatchedTrashDirectories;
	BVolumeRoster fVolumeRoster;
};

#endif
