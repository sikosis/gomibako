#ifndef TRASH_REPLICANT_H
#define TRASH_REPLICANT_H

#include <View.h>
#include <String.h>

class BBitmap;

class TrashReplicant : public BView {
public:
	TrashReplicant(BRect frame);
	TrashReplicant(BMessage* archive);
	~TrashReplicant() override;

	static TrashReplicant* Instantiate(BMessage* archive);

	status_t Archive(BMessage* archive, bool deep = true) const override;
	void AttachedToWindow() override;
	void Draw(BRect updateRect) override;
	void MessageReceived(BMessage* message) override;
	void MouseDown(BPoint where) override;

private:
	void _Initialise();
	void _ShowContextMenu(BPoint screenPoint);

	int32 fCount;
	BBitmap* fIcon;
	bool fShowCount;
	BString fScheduleDescription;
};

#endif
