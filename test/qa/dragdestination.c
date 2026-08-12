#include <stdio.h>
#include <string.h>

#include "qa.h"

static uiLabel *status;
static uiMultilineEntry *dropArea;

static void setStatus(const char *text)
{
	uiLabelSetText(status, text);
}

static uiDragOperation onEnter(uiDragDestination *sender, uiDragContext *dc, void *senderData)
{
	setStatus("Drag entered.");
	return uiDragOperationCopy;
}

static uiDragOperation onMove(uiDragDestination *sender, uiDragContext *dc, void *senderData)
{
	int x, y;
	char buf[256];

	uiDragContextPosition(dc, &x, &y);
	snprintf(buf, sizeof(buf), "Drag moved to (%d, %d).", x, y);
	setStatus(buf);

	return uiDragDestinationLastDragOperation(sender);
}

static void onExit(uiDragDestination *sender, void *senderData)
{
	setStatus("Drag left the drop area.");
}

static int onDrop(uiDragDestination *sender, uiDragContext *dc, void *senderData)
{
	int types;
	int dropped = 0;
	char buf[1024];
	int n = 0;

	types = uiDragContextDragTypes(dc);

	n = snprintf(buf, sizeof(buf), "Dropped.");

	if (types & uiDragTypeText) {
		uiDragData *data = uiDragContextDragData(dc, uiDragTypeText);

		if (data != NULL) {
			n += snprintf(buf + n, sizeof(buf) - n, "\nText: %s", data->data.text);
			uiFreeDragData(data);
			dropped = 1;
		}
	}

	if (types & uiDragTypeURIs) {
		uiDragData *data = uiDragContextDragData(dc, uiDragTypeURIs);

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

	setStatus(buf);
	uiMultilineEntrySetText(dropArea, buf);

	return 1;
}

const char *dragDestinationDropGuide(void) {
	return
	"1.\tDrag a selected piece of text from another application onto the\n"
	"\tmultiline entry. While dragging over the entry, the status label\n"
	"\tshould read `Drag entered.` and then `Drag moved to (x, y).`\n"
	"\n"
	"2.\tRelease the mouse button to drop the text. The status label should\n"
	"\tread `Dropped.` followed by a `Text:` line containing the dropped\n"
	"\ttext, and the same text should appear in the multiline entry.\n"
	"\n"
	"3.\tDrag one or more files from your file manager onto the entry.\n"
	"\tThe status label should read `Dropped.` followed by a `Files:` line\n"
	"\tand one line per dropped file path.\n"
	"\n"
	"4.\tDrag a selection over the entry and then move the cursor outside\n"
	"\tof the entry before releasing. The status label should read\n"
	"\t`Drag left the drop area.` and no drop should occur.";
}

uiControl *dragDestinationDrop(void)
{
	uiBox *vbox;
	uiDragDestination *dd;

	vbox = uiNewVerticalBox();
	uiBoxSetPadded(vbox, 1);

	dropArea = uiNewMultilineEntry();
	uiMultilineEntrySetText(dropArea, "Drop text or files here.");
	uiMultilineEntrySetReadOnly(dropArea, 1);
	uiBoxAppend(vbox, uiControl(dropArea), 1);

	status = uiNewLabel("No drag yet.");
	uiBoxAppend(vbox, uiControl(status), 0);

	dd = uiNewDragDestination();
	uiDragDestinationSetAcceptTypes(dd, uiDragTypeText | uiDragTypeURIs);
	uiDragDestinationOnEnter(dd, onEnter, NULL);
	uiDragDestinationOnMove(dd, onMove, NULL);
	uiDragDestinationOnExit(dd, onExit, NULL);
	uiDragDestinationOnDrop(dd, onDrop, NULL);
	uiControlRegisterDragDestination(uiControl(dropArea), dd);

	return uiControl(vbox);
}
