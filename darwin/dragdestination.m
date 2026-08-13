// 2 aug 2026
#import "uipriv_darwin.h"
#import <objc/runtime.h>

// The drag destination is attached to the NSView of the control that
// uiControlRegisterDragDestination() was called on. There is no generic
// view->uiControl mapping on macOS, so the uiControl is associated with
// the view via the Objective-C runtime and looked up from the
// NSDraggingDestination methods below.
static char uiprivDragDestinationControlKey;

static void *uiprivDragDestinationControlKeyPtr(void)
{
	return &uiprivDragDestinationControlKey;
}

void uiprivAssociateDragDestination(uiControl *c)
{
	objc_setAssociatedObject((id) uiControlHandle(c),
		uiprivDragDestinationControlKeyPtr(), c, OBJC_ASSOCIATION_ASSIGN);
}

void uiprivDisassociateDragDestination(uiControl *c)
{
	objc_setAssociatedObject((id) uiControlHandle(c),
		uiprivDragDestinationControlKeyPtr(), nil, OBJC_ASSOCIATION_ASSIGN);
}

static uiControl *uiprivDragDestinationControlForView(NSView *view)
{
	return (uiControl *) objc_getAssociatedObject(view, uiprivDragDestinationControlKeyPtr());
}

static NSDragOperation uiprivRunDragDestination(NSView *view, id<NSDraggingInfo> info, BOOL move)
{
	uiControl *c = uiprivDragDestinationControlForView(view);
	uiDragDestination *dd;
	uiDragContext dc = { info, view };
	uiDragOperation op;

	if (c == NULL || c->dragDest == NULL)
		return NSDragOperationNone;
	dd = c->dragDest;

	if (move)
		op = dd->onMove(dd, &dc, dd->onMoveData);
	else {
		dd->op = uiDragOperationNone;
		op = dd->onEnter(dd, &dc, dd->onEnterData);
	}
	dd->op = op;

	return uiprivDragOperationToNSDragOperation(op);
}

NSDragOperation uiprivDragOperationToNSDragOperation(uiDragOperation op)
{
	switch (op) {
		case uiDragOperationNone:
			return NSDragOperationNone;
		case uiDragOperationCopy:
			return NSDragOperationCopy;
		case uiDragOperationLink:
			return NSDragOperationLink;
		case uiDragOperationMove:
			return NSDragOperationMove;
	}
	return NSDragOperationNone;
}

// The drag destination callbacks are implemented as a category on NSView
// so every control that conforms to NSDraggingDestination responds to
// them. Controls that were never registered as a drag destination have no
// associated uiControl and simply reject the drag.
@implementation NSView (uiprivDragDestination)

- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender
{
	return uiprivRunDragDestination(self, sender, NO);
}

- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender
{
	return uiprivRunDragDestination(self, sender, YES);
}

- (void)draggingExited:(id<NSDraggingInfo>)sender
{
	uiControl *c = uiprivDragDestinationControlForView(self);

	if (c == NULL || c->dragDest == NULL)
		return;
	c->dragDest->onExit(c->dragDest, c->dragDest->onExitData);
}

- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender
{
	uiControl *c = uiprivDragDestinationControlForView(self);

	if (c == NULL || c->dragDest == NULL)
		return NO;
	return uiDragDestinationLastDragOperation(c->dragDest) != uiDragOperationNone;
}

- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender
{
	uiControl *c = uiprivDragDestinationControlForView(self);
	uiDragContext dc = { sender, self };

	if (c == NULL || c->dragDest == NULL)
		return NO;
	return c->dragDest->onDrop(c->dragDest, &dc, c->dragDest->onDropData);
}

@end
