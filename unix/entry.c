// 11 june 2015
#include "uipriv_unix.h"

struct uiEntry {
	uiUnixControl c;
	GtkWidget *widget;
	GtkEntry *entry;
	GtkEditable *editable;
	void (*onChanged)(uiEntry *, void *);
	void *onChangedData;
	gulong onChangedSignal;
	void (*onFilesDropped)(uiEntry *, int, char **, void *);
	void *onFilesDroppedData;
	int acceptDrops;
};

uiUnixControlAllDefaults(uiEntry)

static void onChanged(GtkEditable *editable, gpointer data)
{
	uiEntry *e = uiEntry(data);

	(*(e->onChanged))(e, e->onChangedData);
}

static void defaultOnChanged(uiEntry *e, void *data)
{
	// do nothing
}

char *uiEntryText(uiEntry *e)
{
	return uiUnixStrdupText(gtk_entry_get_text(e->entry));
}

void uiEntrySetText(uiEntry *e, const char *text)
{
	// we need to inhibit sending of ::changed because this WILL send a ::changed otherwise
	g_signal_handler_block(e->editable, e->onChangedSignal);
	gtk_entry_set_text(e->entry, text);
	g_signal_handler_unblock(e->editable, e->onChangedSignal);
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
	return gtk_editable_get_editable(e->editable) == FALSE;
}

void uiEntrySetReadOnly(uiEntry *e, int readonly)
{
	gboolean editable;

	editable = TRUE;
	if (readonly)
		editable = FALSE;
	gtk_editable_set_editable(e->editable, editable);
}

char *uiEntryPlaceholder(uiEntry *e)
{
	const char *text = gtk_entry_get_placeholder_text(e->entry);
	if (!text)
		return uiUnixStrdupText("");
	return uiUnixStrdupText(text);
}

void uiEntrySetPlaceholder(uiEntry *e, const char *text)
{
	gtk_entry_set_placeholder_text(e->entry, text);
}

static uiEntry *finishNewEntry(GtkWidget *w, const gchar *signal)
{
	uiEntry *e;

	uiUnixNewControl(uiEntry, e);

	e->widget = w;
	e->entry = GTK_ENTRY(e->widget);
	e->editable = GTK_EDITABLE(e->widget);

	e->onChangedSignal = g_signal_connect(e->widget, signal, G_CALLBACK(onChanged), e);
	uiEntryOnChanged(e, defaultOnChanged, NULL);
	uiEntryOnFilesDropped(e, defaultOnFilesDropped, NULL);
	uiEntrySetAcceptDrops(e, 0);

	return e;
}

uiEntry *uiNewEntry(void)
{
	return finishNewEntry(gtk_entry_new(), "changed");
}

uiEntry *uiNewPasswordEntry(void)
{
	GtkWidget *e;

	e = gtk_entry_new();
	gtk_entry_set_visibility(GTK_ENTRY(e), FALSE);
	return finishNewEntry(e, "changed");
}

// TODO make it use a separate function to be type-safe
uiEntry *uiNewSearchEntry(void)
{
	return finishNewEntry(gtk_search_entry_new(), "search-changed");
}
