#ifndef GOMIBAKO_APP_H
#define GOMIBAKO_APP_H

#include <Application.h>
#include <Messenger.h>
#include <Node.h>
#include <String.h>
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
	bool _ConfirmAction(const char* title, const char* text,
		const char* actionLabel);
	void _HandleCommandFinished(BMessage* message);
	void _LaunchTrashCommand(const char* executable,
		const std::vector<BString>& arguments, const char* actionName);
	void _OpenTrash();
	void _RebuildTrashWatches();
	void _RefreshTrashCount();
	void _RemoveDeskbarItem();
	void _SendTrashState();
	void _TrashRefs(BMessage* message);

	bool fDeskbarItemAdded;
	bool fTrashActionRunning;
	BMessenger fReplicant;
	int32 fTrashCount;
	std::vector<node_ref> fWatchedTrashDirectories;
	BVolumeRoster fVolumeRoster;
};

#endif
