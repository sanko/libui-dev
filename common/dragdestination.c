// 2 aug 2026
#include "../ui.h"
#include "uipriv.h"

static uiDragOperation defaultOnEnter(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	return uiDragOperationNone;
}

static uiDragOperation defaultOnMove(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	return uiDragDestinationLastDragOperation(dd);
}

static void defaultOnExit(uiDragDestination *dd, void *data)
{
	// do nothing
}

static int defaultOnDrop(uiDragDestination *dd, uiDragContext *dc, void *senderData)
{
	return 0;
}

uiDragDestination *uiNewDragDestination(void)
{
	uiDragDestination *dd;

	dd = uiprivNew(uiDragDestination);
	dd->typeMask = 0;

	uiDragDestinationOnEnter(dd, defaultOnEnter, NULL);
	uiDragDestinationOnMove(dd, defaultOnMove, NULL);
	uiDragDestinationOnExit(dd, defaultOnExit, NULL);
	uiDragDestinationOnDrop(dd, defaultOnDrop, NULL);

	return dd;
}

void uiDragDestinationOnEnter(uiDragDestination *dd, uiDragOperation (*f)(uiDragDestination *, uiDragContext *, void *), void *data)
{
	dd->onEnter = f;
	dd->onEnterData = data;
}

void uiDragDestinationOnMove(uiDragDestination *dd, uiDragOperation (*f)(uiDragDestination *, uiDragContext *, void *), void *data)
{
	dd->onMove = f;
	dd->onMoveData = data;
}

void uiDragDestinationOnExit(uiDragDestination *dd, void (*f)(uiDragDestination *, void *), void *data)
{
	dd->onExit = f;
	dd->onExitData = data;
}

void uiDragDestinationOnDrop(uiDragDestination *dd, int (*f)(uiDragDestination *, uiDragContext *, void *), void *data)
{
	dd->onDrop = f;
	dd->onDropData = data;
}

void uiDragDestinationSetAcceptTypes(uiDragDestination *dd, int typeMask)
{
	dd->typeMask = typeMask;
}

int uiDragDestinationAcceptTypes(uiDragDestination *dd)
{
	return dd->typeMask;
}

uiDragOperation uiDragDestinationLastDragOperation(uiDragDestination *dd)
{
	return dd->op;
}
