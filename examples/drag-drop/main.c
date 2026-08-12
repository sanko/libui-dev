// 2 aug 2026

// A simple drag-and-drop destination demo. Drag files or text onto the
// multiline entry; the accepted types, cursor position and dropped content
// are shown in the status label.

#include <stdio.h>
#include <string.h>
#include <ui.h>

static uiLabel *status;
static uiMultilineEntry *dropArea;

static void setStatus(const char *text)
{
	uiLabelSetText(status, text);
}

static uiDragOperation onEnter(uiDragDestination *sender, uiDragContext *dc, void *senderData)
{
	int types = uiDragContextDragTypes(dc);
	char buf[256];

	// pick the first operation the source offers, preferring Copy
	if (uiDragContextDragOperations(dc) & uiDragOperationCopy)
		snprintf(buf, sizeof(buf), "Drag entered. Available types: %s",
			(types & uiDragTypeText) && (types & uiDragTypeURIs) ? "text and files" :
			(types & uiDragTypeText) ? "text" :
			(types & uiDragTypeURIs) ? "files" : "none");
	else
		snprintf(buf, sizeof(buf), "Drag entered, but the source does not offer a copy operation.");
	setStatus(buf);

	return uiDragOperationCopy;
}

static uiDragOperation onMove(uiDragDestination *sender, uiDragContext *dc, void *senderData)
{
	int x, y;
	char buf[256];

	uiDragContextPosition(dc, &x, &y);
	snprintf(buf, sizeof(buf), "Drag moved to (%d, %d).", x, y);
	setStatus(buf);

	// the operation we chose in onEnter stays active
	return uiDragDestinationLastDragOperation(sender);
}

static void onExit(uiDragDestination *sender, void *senderData)
{
	setStatus("Drag left the drop area.");
}

static int onDrop(uiDragDestination *sender, uiDragContext *dc, void *senderData)
{
	int types;
	uiDragData *data;
	char buf[1024];
	int n = 0;
	int dropped = 0;

	types = uiDragContextDragTypes(dc);

	n = snprintf(buf, sizeof(buf), "Dropped on the drop area.");

	if (types & uiDragTypeText) {
		data = uiDragContextDragData(dc, uiDragTypeText);
		if (data != NULL) {
			n += snprintf(buf + n, sizeof(buf) - n, "\nText: %s", data->data.text);
			uiFreeDragData(data);
			dropped = 1;
		}
	}

	if (types & uiDragTypeURIs) {
		data = uiDragContextDragData(dc, uiDragTypeURIs);
		if (data != NULL) {
			int i;

			n += snprintf(buf + n, sizeof(buf) - n, "\nFiles:");
			for (i = 0; i < data->data.URIs.numURIs; i++)
				n += snprintf(buf + n, sizeof(buf) - n, "\n%s", data->data.URIs.URIs[i]);
			uiFreeDragData(data);
			dropped = 1;
		}
	}

	if (!dropped)
		snprintf(buf, sizeof(buf), "Drop rejected: no supported data was offered.");

	uiMultilineEntrySetText(dropArea, "");
	setStatus(buf);

	// return true to accept the drop
	return 1;
}

static int onClosing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

int main(void)
{
	uiInitOptions o = {0};
	const char *err;
	uiWindow *w;
	uiBox *box;
	uiDragDestination *dd;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "error initializing libui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	w = uiNewWindow("Drag and Drop", 480, 360, 0);
	uiWindowSetMargined(w, 1);
	uiWindowOnClosing(w, onClosing, NULL);

	box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);
	uiWindowSetChild(w, uiControl(box));

	dropArea = uiNewMultilineEntry();
	uiMultilineEntrySetText(dropArea,
		"Drag text or files from another application and drop them here.");
	uiMultilineEntrySetReadOnly(dropArea, 1);
	uiBoxAppend(box, uiControl(dropArea), 1);

	status = uiNewLabel("No drag yet.");
	uiBoxAppend(box, uiControl(status), 0);

	dd = uiNewDragDestination();
	uiDragDestinationSetAcceptTypes(dd, uiDragTypeText | uiDragTypeURIs);
	uiDragDestinationOnEnter(dd, onEnter, NULL);
	uiDragDestinationOnMove(dd, onMove, NULL);
	uiDragDestinationOnExit(dd, onExit, NULL);
	uiDragDestinationOnDrop(dd, onDrop, NULL);
	uiControlRegisterDragDestination(uiControl(dropArea), dd);

	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
