// 2 aug 2026
#import "uipriv_darwin.h"

void uiDragContextPosition(uiDragContext *dc, int *x, int *y)
{
	NSPoint pt = [dc->info draggingLocation];
	*x = pt.x;
	*y = dc->view.frame.size.height - pt.y;
}

int uiDragContextDragTypes(uiDragContext *dc)
{
	int types = 0;
	NSPasteboard *pboard = [dc->info draggingPasteboard];

	if ([[pboard types] containsObject:NSPasteboardTypeString])
		types |= uiDragTypeText;
	if ([[pboard types] containsObject:NSPasteboardTypeFileURL])
		types |= uiDragTypeURIs;

	return types;
}

int uiDragContextDragOperations(uiDragContext *dc)
{
	int ops = uiDragOperationNone;
	NSDragOperation mask = [dc->info draggingSourceOperationMask];

	if (mask & NSDragOperationCopy)
		ops |= uiDragOperationCopy;
	if (mask & NSDragOperationLink)
		ops |= uiDragOperationLink;
	if (mask & NSDragOperationMove)
		ops |= uiDragOperationMove;

	return ops;
}

uiDragData *uiDragContextDragData(uiDragContext *dc, uiDragType type)
{
	uiDragData *d = NULL;
	NSPasteboard *pboard = [dc->info draggingPasteboard];

	switch (type) {
	case uiDragTypeURIs:
		{
			if ([[pboard types] containsObject:NSPasteboardTypeFileURL]) {
				int i;
				NSArray *urls = [pboard readObjectsForClasses:@[[NSURL class]] options:nil];

				// TODO inform about failure?
				if (urls == nil)
					return NULL;

				d = uiprivNew(uiDragData);
				d->type = uiDragTypeURIs;
				d->data.URIs.numURIs = [urls count];
				d->data.URIs.URIs = uiprivAlloc(d->data.URIs.numURIs * sizeof(*d->data.URIs.URIs), "uiDragDropData->data.URIs.URIs");
				for (i = 0; i < d->data.URIs.numURIs; ++i)
					d->data.URIs.URIs[i] = uiDarwinNSStringToText([urls[i] path]);
			}
		}
		break;
	case uiDragTypeText:
		{
			if ([[pboard types] containsObject:NSPasteboardTypeString]) {
				NSString *text = [pboard stringForType:NSPasteboardTypeString];

				// TODO inform about failure?
				if (text == nil)
					return NULL;

				d = uiprivNew(uiDragData);
				d->type = uiDragTypeText;
				d->data.text = uiDarwinNSStringToText(text);
			}
		}
		break;
	}
	return d;
}
