// 11 august 2026
#include "uipriv_windows.hpp"

#define uiScrollSignature 0x5363726F

struct scrollParams {
	int *pos;
	int pagesize;
	int length;
	int *wheelCarry;
	UINT wheelSPIAction;
};

struct uiScroll {
	uiWindowsControl c;
	HWND hwnd;
	struct uiControl *child;
	int hscrollpos;
	int vscrollpos;
	int hwheelCarry;
	int vwheelCarry;
	int childWidth;
	int childHeight;
	int viewW;
	int viewH;
};

static void scrollto(uiScroll *s, int which, struct scrollParams *p, int pos)
{
	SCROLLINFO si;

	// note that the pos < 0 check is /after/ the p->length - p->pagesize check
	// it used to be /before/; this was actually a bug in Raymond Chen's original algorithm: if there are fewer than a page's worth of items, p->length - p->pagesize will be negative and our content draw at the bottom of the window
	// this SHOULD have the same effect with that bug fixed and no others introduced... (thanks to devin on irc.badnik.net for confirming this logic)
	if (pos > p->length - p->pagesize)
		pos = p->length - p->pagesize;
	if (pos < 0)
		pos = 0;

	*(p->pos) = pos;

	// now commit our new scrollbar setup...
	ZeroMemory(&si, sizeof (SCROLLINFO));
	si.cbSize = sizeof (SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	si.nPage = p->pagesize;
	si.nMin = 0;
	si.nMax = p->length - 1;		// endpoint inclusive
	si.nPos = *(p->pos);
	SetScrollInfo(s->hwnd, which, &si, TRUE);

	// and move the child to match
	uiWindowsEnsureMoveWindowDuringResize((HWND) uiControlHandle(s->child),
		-s->hscrollpos, -s->vscrollpos, s->childWidth, s->childHeight);
}

static void scrollby(uiScroll *s, int which, struct scrollParams *p, int delta)
{
	scrollto(s, which, p, *(p->pos) + delta);
}

static void scroll(uiScroll *s, int which, struct scrollParams *p, WPARAM wParam, LPARAM lParam)
{
	int pos;
	SCROLLINFO si;

	pos = *(p->pos);
	switch (LOWORD(wParam)) {
	case SB_LEFT:			// also SB_TOP
		pos = 0;
		break;
	case SB_RIGHT:			// also SB_BOTTOM
		pos = p->length - p->pagesize;
		break;
	case SB_LINELEFT:		// also SB_LINEUP
		pos--;
		break;
	case SB_LINERIGHT:		// also SB_LINEDOWN
		pos++;
		break;
	case SB_PAGELEFT:		// also SB_PAGEUP
		pos -= p->pagesize;
		break;
	case SB_PAGERIGHT:		// also SB_PAGEDOWN
		pos += p->pagesize;
		break;
	case SB_THUMBPOSITION:
		ZeroMemory(&si, sizeof (SCROLLINFO));
		si.cbSize = sizeof (SCROLLINFO);
		si.fMask = SIF_POS;
		if (GetScrollInfo(s->hwnd, which, &si) == 0)
			logLastError(L"error getting thumb position for scroll");
		pos = si.nPos;
		break;
	case SB_THUMBTRACK:
		ZeroMemory(&si, sizeof (SCROLLINFO));
		si.cbSize = sizeof (SCROLLINFO);
		si.fMask = SIF_TRACKPOS;
		if (GetScrollInfo(s->hwnd, which, &si) == 0)
			logLastError(L"error getting thumb track position for scroll");
		pos = si.nTrackPos;
		break;
	}
	scrollto(s, which, p, pos);
}

static void wheelscroll(uiScroll *s, int which, struct scrollParams *p, WPARAM wParam, LPARAM lParam)
{
	int delta;
	int lines;
	UINT scrollAmount;

	if (p->pagesize <= 0 || p->pagesize >= p->length)		// nothing to scroll to
		return;
	delta = GET_WHEEL_DELTA_WPARAM(wParam);
	if (SystemParametersInfoW(p->wheelSPIAction, 0, &scrollAmount, 0) == 0)
		// TODO use scrollAmount == 3 (for both v and h) instead?
		logLastError(L"error getting scroll wheel scroll amount");
	if (scrollAmount == WHEEL_PAGESCROLL)
		scrollAmount = p->pagesize;
	if (scrollAmount == 0)		// no mouse wheel scrolling (or pagesize == 0)
		return;
	// the rest of this is basically http://blogs.msdn.com/b/oldnewthing/archive/2003/08/07/54615.aspx and http://blogs.msdn.com/b/oldnewthing/archive/2003/08/11/54624.aspx
	// see those pages for information on subtleties
	delta += *(p->wheelCarry);
	lines = delta * ((int) scrollAmount) / WHEEL_DELTA;
	*(p->wheelCarry) = delta - lines * WHEEL_DELTA / ((int) scrollAmount);
	scrollby(s, which, p, -lines);
}

static void hscrollParams(uiScroll *s, struct scrollParams *p)
{
	ZeroMemory(p, sizeof (struct scrollParams));
	p->pos = &(s->hscrollpos);
	p->pagesize = s->viewW;
	p->length = s->childWidth;
	p->wheelCarry = &(s->hwheelCarry);
	p->wheelSPIAction = SPI_GETWHEELSCROLLCHARS;
}

static void vscrollParams(uiScroll *s, struct scrollParams *p)
{
	ZeroMemory(p, sizeof (struct scrollParams));
	p->pos = &(s->vscrollpos);
	p->pagesize = s->viewH;
	p->length = s->childHeight;
	p->wheelCarry = &(s->vwheelCarry);
	p->wheelSPIAction = SPI_GETWHEELSCROLLLINES;
}

static void hscroll(uiScroll *s, WPARAM wParam, LPARAM lParam)
{
	struct scrollParams p;

	hscrollParams(s, &p);
	scroll(s, SB_HORZ, &p, wParam, lParam);
}

static void hwheelscroll(uiScroll *s, WPARAM wParam, LPARAM lParam)
{
	struct scrollParams p;

	hscrollParams(s, &p);
	wheelscroll(s, SB_HORZ, &p, wParam, lParam);
}

static void vscroll(uiScroll *s, WPARAM wParam, LPARAM lParam)
{
	struct scrollParams p;

	vscrollParams(s, &p);
	scroll(s, SB_VERT, &p, wParam, lParam);
}

static void vwheelscroll(uiScroll *s, WPARAM wParam, LPARAM lParam)
{
	struct scrollParams p;

	vscrollParams(s, &p);
	wheelscroll(s, SB_VERT, &p, wParam, lParam);
}

static BOOL scrollDoScroll(uiScroll *s, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *lResult)
{
	switch (uMsg) {
	case WM_HSCROLL:
	case WM_VSCROLL:
		// a non-NULL lParam means this came from a child control (like a trackbar), not from our own scrollbar
		if (lParam != 0)
			return FALSE;
		if (uMsg == WM_HSCROLL)
			hscroll(s, wParam, lParam);
		else
			vscroll(s, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_MOUSEHWHEEL:
		hwheelscroll(s, wParam, lParam);
		*lResult = 0;
		return TRUE;
	case WM_MOUSEWHEEL:
		vwheelscroll(s, wParam, lParam);
		*lResult = 0;
		return TRUE;
	}
	return FALSE;
}

static void scrollRange(uiScroll *s, int which, int view, int length, int *pos)
{
	SCROLLINFO si;

	if (length <= view) {
		*pos = 0;
		return;
	}
	if (*pos > length - view)
		*pos = length - view;
	if (*pos < 0)
		*pos = 0;

	ZeroMemory(&si, sizeof (SCROLLINFO));
	si.cbSize = sizeof (SCROLLINFO);
	si.fMask = SIF_PAGE | SIF_POS | SIF_RANGE;
	si.nPage = view;
	si.nMin = 0;
	si.nMax = length - 1;		// endpoint inclusive
	si.nPos = *pos;
	SetScrollInfo(s->hwnd, which, &si, TRUE);
}

static void scrollRelayout(uiScroll *s)
{
	RECT r;
	int clientW, clientH;
	int childMinW, childMinH;
	int viewW, viewH;
	BOOL hasH, hasV;
	int vscrollW, hscrollH;

	if (s->child == NULL) {
		s->hscrollpos = 0;
		s->vscrollpos = 0;
		s->childWidth = 0;
		s->childHeight = 0;
		s->viewW = 0;
		s->viewH = 0;
		ShowScrollBar(s->hwnd, SB_HORZ, FALSE);
		ShowScrollBar(s->hwnd, SB_VERT, FALSE);
		return;
	}

	uiWindowsEnsureGetClientRect(s->hwnd, &r);
	clientW = r.right - r.left;
	clientH = r.bottom - r.top;

	uiWindowsControlMinimumSize(uiWindowsControl(s->child), &childMinW, &childMinH);

	vscrollW = GetSystemMetrics(SM_CXVSCROLL);
	hscrollH = GetSystemMetrics(SM_CYHSCROLL);

	viewW = clientW;
	viewH = clientH;
	s->childWidth = childMinW;
	s->childHeight = childMinH;

	hasH = s->childWidth > viewW;
	hasV = s->childHeight > viewH;

	// showing one scrollbar shrinks the viewport, which may make the other one necessary
	if (hasH || hasV) {
		if (hasV)
			viewW = clientW - vscrollW;
		if (hasH)
			viewH = clientH - hscrollH;
		s->childWidth = childMinW > viewW ? childMinW : viewW;
		s->childHeight = childMinH > viewH ? childMinH : viewH;
		hasH = s->childWidth > viewW;
		hasV = s->childHeight > viewH;
	}

	s->viewW = viewW;
	s->viewH = viewH;

	ShowScrollBar(s->hwnd, SB_HORZ, hasH);
	ShowScrollBar(s->hwnd, SB_VERT, hasV);

	if (hasH)
		scrollRange(s, SB_HORZ, viewW, s->childWidth, &(s->hscrollpos));
	else
		s->hscrollpos = 0;
	if (hasV)
		scrollRange(s, SB_VERT, viewH, s->childHeight, &(s->vscrollpos));
	else
		s->vscrollpos = 0;

	uiWindowsEnsureMoveWindowDuringResize((HWND) uiControlHandle(s->child),
		-s->hscrollpos, -s->vscrollpos, s->childWidth, s->childHeight);
}

static void onResize(uiWindowsControl *c)
{
	scrollRelayout(uiScroll(c));
}

static LRESULT CALLBACK scrollSubProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	uiScroll *s = uiScroll(dwRefData);
	LRESULT lResult;

	if (handleParentMessages(hwnd, uMsg, wParam, lParam, &lResult) != FALSE)
		return lResult;
	if (scrollDoScroll(s, uMsg, wParam, lParam, &lResult))
		return lResult;
	switch (uMsg) {
	case WM_NCDESTROY:
		if (RemoveWindowSubclass(hwnd, scrollSubProc, uIdSubclass) == FALSE)
			logLastError(L"error removing scroll container subclass");
		break;
	}
	return DefSubclassProc(hwnd, uMsg, wParam, lParam);
}

static void uiScrollDestroy(uiControl *c)
{
	uiScroll *s = uiScroll(c);

	if (s->child != NULL) {
		uiControlSetParent(s->child, NULL);
		uiControlDestroy(s->child);
	}
	uiprivDestroyTooltip(c);
	uiWindowsEnsureDestroyWindow(s->hwnd);
	uiFreeControl(uiControl(s));
}

uiWindowsControlDefaultHandle(uiScroll)
uiWindowsControlDefaultParent(uiScroll)
uiWindowsControlDefaultSetParent(uiScroll)
uiWindowsControlDefaultToplevel(uiScroll)
uiWindowsControlDefaultVisible(uiScroll)
uiWindowsControlDefaultShow(uiScroll)
uiWindowsControlDefaultHide(uiScroll)
uiWindowsControlDefaultEnabled(uiScroll)
uiWindowsControlDefaultEnable(uiScroll)
uiWindowsControlDefaultDisable(uiScroll)

static void uiScrollSyncEnableState(uiWindowsControl *c, int enabled)
{
	uiScroll *s = uiScroll(c);

	if (uiWindowsShouldStopSyncEnableState(uiWindowsControl(s), enabled))
		return;
	EnableWindow(s->hwnd, enabled);
	if (s->child != NULL)
		uiWindowsControlSyncEnableState(uiWindowsControl(s->child), enabled);
}

uiWindowsControlDefaultSetParentHWND(uiScroll)

static void uiScrollMinimumSize(uiWindowsControl *c, int *width, int *height)
{
	uiScroll *s = uiScroll(c);
	int vscrollW, hscrollH;

	*width = 0;
	*height = 0;
	if (s->child != NULL)
		uiWindowsControlMinimumSize(uiWindowsControl(s->child), width, height);
	// we only need to reserve space for the scrollbars; the child can be scrolled into view
	// the control will still expand to fill any available space
	vscrollW = GetSystemMetrics(SM_CXVSCROLL);
	hscrollH = GetSystemMetrics(SM_CYHSCROLL);
	if (*width < vscrollW)
		*width = vscrollW;
	if (*height < hscrollH)
		*height = hscrollH;
}

static void uiScrollMinimumSizeChanged(uiWindowsControl *c)
{
	uiScroll *s = uiScroll(c);

	if (uiWindowsControlTooSmall(uiWindowsControl(s))) {
		uiWindowsControlContinueMinimumSizeChanged(uiWindowsControl(s));
		return;
	}
	scrollRelayout(s);
}

uiWindowsControlDefaultLayoutRect(uiScroll)
uiWindowsControlDefaultAssignControlIDZOrder(uiScroll)

uiWindowsControlRelayoutOnChildVisibilityChanged(uiScroll)

void uiScrollSetChild(uiScroll *s, uiControl *child)
{
	if (s->child != NULL) {
		uiControlSetParent(s->child, NULL);
		uiWindowsControlSetParentHWND(uiWindowsControl(s->child), NULL);
	}
	s->child = child;
	if (s->child != NULL) {
		uiControlSetParent(s->child, uiControl(s));
		uiWindowsControlSetParentHWND(uiWindowsControl(s->child), s->hwnd);
		uiWindowsControlAssignSoleControlIDZOrder(uiWindowsControl(s->child));
		s->hscrollpos = 0;
		s->vscrollpos = 0;
		uiWindowsControlMinimumSizeChanged(uiWindowsControl(s));
	}
}

uiScroll *uiNewScroll(void)
{
	uiScroll *s;

	uiWindowsNewControl(uiScroll, s);

	s->hwnd = uiWindowsMakeContainer(uiWindowsControl(s), onResize);

	s->hscrollpos = 0;
	s->vscrollpos = 0;
	s->hwheelCarry = 0;
	s->vwheelCarry = 0;
	s->childWidth = 0;
	s->childHeight = 0;
	s->viewW = 0;
	s->viewH = 0;

	if (SetWindowSubclass(s->hwnd, scrollSubProc, 0, (DWORD_PTR) s) == FALSE)
		logLastError(L"error subclassing scroll container to handle scrollbar messages");

	return s;
}
