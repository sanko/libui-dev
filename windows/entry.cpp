// 8 april 2015
#include "uipriv_windows.hpp"

struct uiEntry {
	uiWindowsControl c;
	HWND hwnd;
	void (*onChanged)(uiEntry *, void *);
	void *onChangedData;
	BOOL inhibitChanged;
	int placeholderLen;
	void (*onFilesDropped)(uiEntry *, int, char **, void *);
	void *onFilesDroppedData;
	int acceptDrops;
};

static BOOL onWM_COMMAND(uiControl *c, HWND hwnd, WORD code, LRESULT *lResult)
{
	uiEntry *e = uiEntry(c);

	if (code != EN_CHANGE)
		return FALSE;
	if (e->inhibitChanged)
		return FALSE;
	(*(e->onChanged))(e, e->onChangedData);
	*lResult = 0;
	return TRUE;
}

static void uiEntryDestroy(uiControl *c)
{
	uiEntry *e = uiEntry(c);

	uiWindowsUnregisterWM_COMMANDHandler(e->hwnd);
	uiprivDestroyTooltip(c);
	uiWindowsEnsureDestroyWindow(e->hwnd);
	uiFreeControl(uiControl(e));
}

uiWindowsControlAllDefaultsExceptDestroy(uiEntry)

// from http://msdn.microsoft.com/en-us/library/windows/desktop/dn742486.aspx#sizingandspacing
#define entryWidth 107 /* this is actually the shorter progress bar width, but Microsoft only indicates as wide as necessary */
#define entryHeight 14

static void uiEntryMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiEntry *e = uiEntry(c);
	uiWindowsSizing sizing;
	int x, y;

	x = entryWidth;
	y = entryHeight;
	uiWindowsGetSizing(e->hwnd, &sizing);
	uiWindowsSizingDlgUnitsToPixels(&sizing, &x, &y);
	*width = x;
	*height = y;
}

static void defaultOnChanged(uiEntry *e, void *data)
{
	// do nothing
}

char *uiEntryText(uiEntry *e)
{
	return uiWindowsWindowText(e->hwnd);
}

void uiEntrySetText(uiEntry *e, const char *text)
{
	// doing this raises an EN_CHANGED
	e->inhibitChanged = TRUE;
	uiWindowsSetWindowText(e->hwnd, text);
	// Only set the cursor if the entry has focus to avoid weird scrolling upon window
	// creation. Cursor placement is otherwise determined by mouse position upon click.
	if (GetFocus() == e->hwnd)
		Edit_SetSel(e->hwnd, (WPARAM) -1, (LPARAM) -1);
	e->inhibitChanged = FALSE;
	// don't queue the control for resize; entry sizes are independent of their contents
}

void uiEntryOnChanged(uiEntry *e, void (*f)(uiEntry *, void *), void *data)
{
	e->onChanged = f;
	e->onChangedData = data;
}

static void defaultOnFilesDropped(uiEntry *e, int fileCount, char **fileNames, void *data)
{
	// do nothing
}

static uiDragOperation entryOnEnter(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	uiEntry *e = (uiEntry *) data;

	if (e->acceptDrops && (uiDragContextDragTypes(dc) & uiDragTypeURIs))
		return uiDragOperationCopy;
	return uiDragOperationNone;
}

static int entryOnDrop(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	uiEntry *e = (uiEntry *) data;
	uiDragData *d;

	if (!e->acceptDrops)
		return 0;
	d = uiDragContextDragData(dc, uiDragTypeURIs);
	if (d == NULL)
		return 0;
	(*(e->onFilesDropped))(e, d->data.URIs.numURIs, d->data.URIs.URIs, e->onFilesDroppedData);
	uiFreeDragData(d);
	return 1;
}

void uiEntryOnFilesDropped(uiEntry *e,
	void (*f)(uiEntry *, int, char **, void *), void *data)
{
	e->onFilesDropped = f;
	e->onFilesDroppedData = data;
}

int uiEntryAcceptDrops(uiEntry *e)
{
	return e->acceptDrops;
}

void uiEntrySetAcceptDrops(uiEntry *e, int accept)
{
	uiDragDestination *dd;

	e->acceptDrops = accept;
	if (!accept || uiControl(e)->dragDest != NULL)
		return;

	dd = uiNewDragDestination();
	uiDragDestinationSetAcceptTypes(dd, uiDragTypeURIs);
	uiDragDestinationOnEnter(dd, entryOnEnter, e);
	uiDragDestinationOnDrop(dd, entryOnDrop, e);
	uiControlRegisterDragDestination(uiControl(e), dd);
}

int uiEntryReadOnly(uiEntry *e)
{
	return (getStyle(e->hwnd) & ES_READONLY) != 0;
}

void uiEntrySetReadOnly(uiEntry *e, int readonly)
{
	if (Edit_SetReadOnly(e->hwnd, readonly) == 0)
		logLastError(L"error setting uiEntry read-only state");
}

char *uiEntryPlaceholder(uiEntry *e)
{
	return uiprivEntryPlaceholder(e->hwnd, e->placeholderLen);
}

void uiEntrySetPlaceholder(uiEntry *e, const char *text)
{
	// This won't work for read-only entries.
	e->placeholderLen = uiprivSetEntryPlaceholder(e->hwnd, text);
}

static uiEntry *finishNewEntry(DWORD style)
{
	uiEntry *e;

	uiWindowsNewControl(uiEntry, e);

	e->hwnd = uiWindowsEnsureCreateControlHWND(WS_EX_CLIENTEDGE,
		L"edit", L"",
		style | ES_AUTOHSCROLL | ES_LEFT | ES_NOHIDESEL | WS_TABSTOP,
		hInstance, NULL,
		TRUE);
	e->placeholderLen = 0;

	uiWindowsRegisterWM_COMMANDHandler(e->hwnd, onWM_COMMAND, uiControl(e));
	uiEntryOnChanged(e, defaultOnChanged, NULL);
	uiEntryOnFilesDropped(e, defaultOnFilesDropped, NULL);
	uiEntrySetAcceptDrops(e, 0);

	return e;
}

uiEntry *uiNewEntry(void)
{
	return finishNewEntry(0);
}

uiEntry *uiNewPasswordEntry(void)
{
	return finishNewEntry(ES_PASSWORD);
}

uiEntry *uiNewSearchEntry(void)
{
	uiEntry *e;
	HRESULT hr;

	e = finishNewEntry(0);

	hr = SetWindowTheme(e->hwnd, L"SearchBoxEditComposited", NULL);
	if (hr != S_OK || !IsAppThemed()) {
		//TODO log: Failed to apply search box theme.
	}

	return e;
}
