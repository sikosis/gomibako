#include "AboutWindow.h"

#include "TrashIcon.h"

#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <Button.h>
#include <File.h>
#include <Font.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Message.h>
#include <Roster.h>
#include <SeparatorView.h>
#include <Size.h>
#include <String.h>
#include <StringView.h>
#include <TabView.h>
#include <View.h>

class IconView : public BView {
public:
	IconView(BBitmap* icon);
	~IconView() override;

	void Draw(BRect updateRect) override;

private:
	BBitmap* fIcon;
};

class LinkView : public BStringView {
public:
	LinkView(const char* name, const char* label, const char* url);

	void AttachedToWindow() override;
	void MouseDown(BPoint point) override;

private:
	BString fUrl;
};

static BString AppVersion() {
	BString version("Version v0.18");
	app_info appInfo;
	if (be_app->GetAppInfo(&appInfo) != B_OK)
		return version;

	BFile appFile(&appInfo.ref, B_READ_ONLY);
	BAppFileInfo appFileInfo(&appFile);
	version_info versionInfo;
	if (appFileInfo.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND) == B_OK) {
		if (versionInfo.minor == 0) {
			version.SetToFormat("Version v%lu.%lu",
				(unsigned long)versionInfo.major,
				(unsigned long)versionInfo.middle);
		} else {
			version.SetToFormat("Version v%lu.%lu%lu",
				(unsigned long)versionInfo.major,
				(unsigned long)versionInfo.middle,
				(unsigned long)versionInfo.minor);
		}
	}

	return version;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static BStringView* Heading(const char* name, const char* text) {
	BStringView* heading = new BStringView(name, text);
	heading->SetFont(be_bold_font);
	return heading;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static BView* CreateCreditsView() {
	BView* view = new BView("Credits", B_WILL_DRAW);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BLayoutBuilder::Group<>(view, B_VERTICAL, 5.0f)
		.SetInsets(12.0f)
		.Add(Heading("project heading", "Project"))
		.Add(new BStringView("copyright",
			"Copyright 2026 The gomibako contributors"))
		.AddStrut(8.0f)
		.Add(Heading("current heading", "Current Contributor"))
		.Add(new BStringView("current contributor", "Sikosis"))
		.AddStrut(8.0f)
		.Add(Heading("thanks heading", "Special Thanks"))
		.Add(new BStringView("haiku thanks",
			"The Haiku project and HaikuArchives community"))
		.AddGlue();

	return view;
}
//---------------------------------------------------------------------------------------------------------------------------------//


static BView* CreateDetailsView() {
	BView* view = new BView("Details", B_WILL_DRAW);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	BLayoutBuilder::Group<>(view, B_VERTICAL, 5.0f)
		.SetInsets(12.0f)
		.Add(Heading("integration heading", "Haiku integration"))
		.Add(new BStringView("deskbar detail",
			"Compact status item hosted by the Haiku Deskbar"))
		.Add(new BStringView("tracker detail",
			"Uses Tracker's native Trash artwork"))
		.AddStrut(8.0f)
		.Add(Heading("planned heading", "Planned features"))
		.Add(new BStringView("planned detail",
			"Settings and automatic emptying"))
		.AddGlue();

	return view;
}
//---------------------------------------------------------------------------------------------------------------------------------//


IconView::IconView(BBitmap* icon)
	:
	BView("application icon", B_WILL_DRAW),
	fIcon(icon)
{
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	SetExplicitMinSize(BSize(48.0f, 48.0f));
	SetExplicitMaxSize(BSize(48.0f, 48.0f));
}
//---------------------------------------------------------------------------------------------------------------------------------//


IconView::~IconView() {
	delete fIcon;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void IconView::Draw(BRect updateRect) {
	(void)updateRect;
	if (fIcon == NULL)
		return;

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
	BPoint position((Bounds().Width() - fIcon->Bounds().Width()) / 2.0f,
		(Bounds().Height() - fIcon->Bounds().Height()) / 2.0f);
	DrawBitmap(fIcon, position);
}
//---------------------------------------------------------------------------------------------------------------------------------//


LinkView::LinkView(const char* name, const char* label, const char* url)
	:
	BStringView(name, label),
	fUrl(url)
{
	SetToolTip(url);
	BFont font;
	GetFont(&font);
	font.SetFace(font.Face() | B_UNDERSCORE_FACE);
	SetFont(&font);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LinkView::AttachedToWindow() {
	BStringView::AttachedToWindow();
	SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	int32 luminance = (299 * background.red + 587 * background.green
		+ 114 * background.blue) / 1000;
	if (luminance < 128)
		SetHighColor(105, 190, 255);
	else
		SetHighColor(0, 76, 168);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void LinkView::MouseDown(BPoint point) {
	(void)point;
	const char* arguments[] = { fUrl.String() };
	be_roster->Launch("text/html", 1, arguments);
}
//---------------------------------------------------------------------------------------------------------------------------------//


AboutWindow::AboutWindow()
	:
	BWindow(BRect(0.0f, 0.0f, 500.0f, 380.0f), "About gomibako",
		B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_CLOSE_ON_ESCAPE)
{
	BStringView* title = new BStringView("title", "gomibako");
	BFont titleFont(*be_bold_font);
	titleFont.SetSize(be_plain_font->Size() * 2.0f);
	title->SetFont(&titleFont);

	BString versionText = AppVersion();
	BStringView* version = new BStringView("version", versionText.String());
	BStringView* description = new BStringView("description",
		"A lightweight Trash manager for the Haiku Deskbar.");

	BTabView* tabs = new BTabView("about tabs", B_WIDTH_FROM_LABEL);
	tabs->AddTab(CreateCreditsView());
	tabs->AddTab(CreateDetailsView());

	BButton* closeButton = new BButton("close", "Close",
		new BMessage(B_QUIT_REQUESTED));
	closeButton->MakeDefault(true);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 10.0f)
		.SetInsets(18.0f)
		.AddGroup(B_HORIZONTAL, 16.0f)
			.Add(new IconView(LoadTrashIcon(32)))
			.AddGroup(B_VERTICAL, 4.0f)
				.Add(title)
				.Add(version)
				.AddStrut(4.0f)
				.Add(description)
				.Add(new LinkView("source link", "github.com/sikosis/gomibako",
					"https://github.com/sikosis/gomibako"))
				.AddGlue()
			.End()
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(tabs, 1.0f)
		.AddGroup(B_HORIZONTAL, 0.0f)
			.AddGlue()
			.Add(closeButton)
		.End();

	CenterOnScreen();
}
//---------------------------------------------------------------------------------------------------------------------------------//
