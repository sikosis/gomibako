#include "TrashIcon.h"

#include <Bitmap.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <IconUtils.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>

static const int32 kTrackerEmptyTrashIcon = 1003;
static const int32 kTrackerFullTrashIcon = 1004;
static const char* const kTrackerSignature = "application/x-vnd.Be-TRAK";

static BBitmap* LoadTrackerTrashIcon(int32 pixelSize, bool full) {
	app_info trackerInfo;
	if (be_roster->GetAppInfo(kTrackerSignature, &trackerInfo) != B_OK)
		return NULL;

	BResources resources(&trackerInfo.ref);
	if (resources.InitCheck() != B_OK)
		return NULL;

	size_t dataSize = 0;
	const uint8* data = static_cast<const uint8*>(resources.LoadResource(
		B_VECTOR_ICON_TYPE,
		full ? kTrackerFullTrashIcon : kTrackerEmptyTrashIcon, &dataSize));
	if (data == NULL || dataSize == 0)
		return NULL;

	BBitmap* icon = new BBitmap(BRect(0.0f, 0.0f, pixelSize - 1.0f,
		pixelSize - 1.0f), B_RGBA32);
	if (icon->InitCheck() != B_OK
		|| BIconUtils::GetVectorIcon(data, dataSize, icon) != B_OK) {
		delete icon;
		return NULL;
	}

	return icon;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static BBitmap* LoadTrashNodeIcon(int32 pixelSize) {
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


BBitmap* LoadTrashIcon(int32 pixelSize, bool full) {
	if (pixelSize != 16 && pixelSize != 32)
		return NULL;

	BBitmap* icon = LoadTrackerTrashIcon(pixelSize, full);
	if (icon != NULL)
		return icon;

	return LoadTrashNodeIcon(pixelSize);
}
//---------------------------------------------------------------------------------------------------------------------------------//
