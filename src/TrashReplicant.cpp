#include "TrashReplicant.h"

#include "Gomibako.h"
#include "TrashIcon.h"

#include <Application.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <Font.h>
#include <MenuItem.h>
#include <Message.h>
#include <Messenger.h>
#include <PopUpMenu.h>

#include <algorithm>
#include <stdio.h>

const char* const kApplicationSignature = "application/x-vnd.sikosis-gomibako";
const char* const kDeskbarItemName = "gomibako";
const char* const kReplicantClassName = "TrashReplicant";

TrashReplicant::TrashReplicant(BRect frame)
	:
	BView(frame, kDeskbarItemName, B_FOLLOW_NONE,
		B_WILL_DRAW | B_TRANSPARENT_BACKGROUND | B_FRAME_EVENTS),
	fCount(0),
	fIcon(NULL)
{
	_Initialise();
}
//---------------------------------------------------------------------------------------------------------------------------------//


TrashReplicant::TrashReplicant(BMessage* archive)
	:
	BView(archive),
	fCount(0),
	fIcon(NULL)
{
	archive->FindInt32("count", &fCount);
	_Initialise();
}
//---------------------------------------------------------------------------------------------------------------------------------//


TrashReplicant::~TrashReplicant() {
	delete fIcon;
}
//---------------------------------------------------------------------------------------------------------------------------------//


TrashReplicant* TrashReplicant::Instantiate(BMessage* archive) {
	if (!validate_instantiation(archive, kReplicantClassName))
		return NULL;

	return new TrashReplicant(archive);
}
//---------------------------------------------------------------------------------------------------------------------------------//


status_t TrashReplicant::Archive(BMessage* archive, bool deep) const {
	status_t status = BView::Archive(archive, deep);
	if (status == B_OK)
		status = archive->AddString("add_on", kApplicationSignature);
	if (status == B_OK)
		status = archive->AddString("class", kReplicantClassName);
	if (status == B_OK)
		status = archive->AddInt32("count", fCount);

	return status;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void TrashReplicant::AttachedToWindow() {
	BView::AttachedToWindow();
	AdoptParentColors();
	SetLowColor(ViewColor());

	BMessage registration(kMessageRegisterReplicant);
	registration.AddMessenger("messenger", BMessenger(this));
	BMessenger(kApplicationSignature).SendMessage(&registration);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void TrashReplicant::Draw(BRect updateRect) {
	(void)updateRect;

	const BRect bounds = Bounds();
	const float iconSize = std::min(16.0f, bounds.Height() - 2.0f);
	const float iconTop = (bounds.Height() - iconSize) / 2.0f;
	if (fIcon != NULL) {
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		DrawBitmap(fIcon, BPoint(1.0f, iconTop));
		SetDrawingMode(B_OP_COPY);
	} else {
		const BRect canRect(2.0f, iconTop + 4.0f, iconSize - 1.0f,
			iconTop + iconSize);
		SetHighUIColor(B_PANEL_TEXT_COLOR);
		StrokeRect(canRect);
		StrokeLine(BPoint(1.0f, iconTop + 3.0f),
			BPoint(iconSize, iconTop + 3.0f));
		StrokeLine(BPoint(5.0f, iconTop + 1.0f),
			BPoint(iconSize - 4.0f, iconTop + 1.0f));
	}

	if (fCount == 0)
		return;

	char countText[16];
	if (fCount > 99)
		snprintf(countText, sizeof(countText), "99+");
	else
		snprintf(countText, sizeof(countText), "%ld", static_cast<long>(fCount));
	SetHighUIColor(B_PANEL_TEXT_COLOR);
	font_height metrics;
	GetFontHeight(&metrics);
	const float baseline = (bounds.Height() - metrics.ascent
		- metrics.descent) / 2.0f + metrics.ascent;
	DrawString(countText, BPoint(iconSize + 5.0f, baseline));
}
//---------------------------------------------------------------------------------------------------------------------------------//


void TrashReplicant::MessageReceived(BMessage* message) {
	switch (message->what) {
		case kMessageTrashState:
		{
			int32 count = 0;
			if (message->FindInt32("count", &count) != B_OK)
				return;

			fCount = std::max(static_cast<int32>(0), count);
			delete fIcon;
			fIcon = LoadTrashIcon(16);
			Invalidate();
			break;
		}

		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
		{
			BMessage request(kMessageTrashRefs);
			entry_ref ref;
			for (int32 index = 0;
				message->FindRef("refs", index, &ref) == B_OK; index++) {
				request.AddRef("refs", &ref);
			}
			BMessenger(kApplicationSignature).SendMessage(&request);
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


void TrashReplicant::MouseDown(BPoint where) {
	BPoint point;
	uint32 buttons = 0;
	GetMouse(&point, &buttons);
	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		ConvertToScreen(&where);
		_ShowContextMenu(where);
		return;
	}

	BMessenger(kApplicationSignature).SendMessage(kMessageOpenTrash);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void TrashReplicant::_Initialise() {
	SetViewColor(B_TRANSPARENT_COLOR);
	SetFont(be_plain_font);
	fIcon = LoadTrashIcon(16);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void TrashReplicant::_ShowContextMenu(BPoint screenPoint) {
	BPopUpMenu* menu = new BPopUpMenu("gomibako", false, false);
	BMenuItem* openItem = new BMenuItem("Open Trash",
		new BMessage(kMessageOpenTrash));
	BMenuItem* emptyItem = new BMenuItem("Empty Trash" B_UTF8_ELLIPSIS,
		new BMessage(kMessageEmptyTrash));
	BMenuItem* restoreItem = new BMenuItem("Restore All" B_UTF8_ELLIPSIS,
		new BMessage(kMessageRestoreTrash));
	BMenuItem* aboutItem = new BMenuItem("About gomibako" B_UTF8_ELLIPSIS,
		new BMessage(B_ABOUT_REQUESTED));
	BMenuItem* removeItem = new BMenuItem("Remove from Deskbar",
		new BMessage(kMessageRemoveFromDeskbar));
	menu->AddItem(openItem);
	menu->AddSeparatorItem();
	menu->AddItem(emptyItem);
	menu->AddItem(restoreItem);
	menu->AddSeparatorItem();
	menu->AddItem(aboutItem);
	menu->AddSeparatorItem();
	menu->AddItem(removeItem);
	menu->SetTargetForItems(BMessenger(kApplicationSignature));
	emptyItem->SetEnabled(fCount > 0);
	restoreItem->SetEnabled(fCount > 0);
	menu->Go(screenPoint, true, true, true);
	delete menu;
}
//---------------------------------------------------------------------------------------------------------------------------------//


extern "C" _EXPORT BView* instantiate_deskbar_item(float maxWidth,
	float maxHeight) {
	const float height = maxHeight;
	const float requestedWidth = height + 18.0f;
	const float width = std::min(maxWidth, requestedWidth);
	return new TrashReplicant(BRect(0.0f, 0.0f, width - 1.0f,
		height - 1.0f));
}
//---------------------------------------------------------------------------------------------------------------------------------//
