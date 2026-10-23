#include "TrashIcon.h"

#include <Bitmap.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>

BBitmap* LoadTrashIcon(int32 pixelSize) {
	if (pixelSize != 16 && pixelSize != 32)
		return NULL;

	BPath trashPath;
	if (find_directory(B_TRASH_DIRECTORY, &trashPath) != B_OK)
		return NULL;

	BEntry trashEntry(trashPath.Path(), true);
	entry_ref trashRef;
	if (trashEntry.GetRef(&trashRef) != B_OK)
		return NULL;

	BBitmap* icon = new BBitmap(BRect(0.0f, 0.0f, pixelSize - 1.0f,
		pixelSize - 1.0f), B_RGBA32);
	if (icon->InitCheck() != B_OK) {
		delete icon;
		return NULL;
	}

	icon_size size = pixelSize == 16 ? B_MINI_ICON : B_LARGE_ICON;
	if (BNodeInfo::GetTrackerIcon(&trashRef, icon, size) != B_OK) {
		delete icon;
		return NULL;
	}

	return icon;
}
//---------------------------------------------------------------------------------------------------------------------------------//
