// 11 august 2026
#import "uipriv_darwin.h"

#define uiScrollSignature 0x5363726F

@interface uiprivScrollView : NSScrollView {
	uiScroll *s;
}
- (id)initWithScroll:(uiScroll *)ss;
@end

// the document view is flipped so that (0, 0) is the top-left corner, matching the other platforms
@interface uiprivScrollDocumentView : NSView
@end

struct uiScroll {
	uiDarwinControl c;
	NSScrollView *sv;
	NSView *doc;
	uiControl *child;
	NSLayoutPriority oldHorzHuggingPri;
	NSLayoutPriority oldVertHuggingPri;
};

static void scrollTile(uiScroll *s);

@implementation uiprivScrollView

- (id)initWithScroll:(uiScroll *)ss
{
	self = [super initWithFrame:NSZeroRect];
	if (self != nil)
		self->s = ss;
	return self;
}

- (void)tile
{
	[super tile];
	if (self->s != nil)
		scrollTile(self->s);
}

- (NSSize)intrinsicContentSize
{
	NSView *childView;

	// when the scroll view is in a container that gives it its natural size (such as
	// a non-stretchy cell of a uiBox), the scroll view should be the size of its content
	if (self->s != nil && self->s->child != nil) {
		childView = (NSView *) uiControlHandle(self->s->child);
		return [childView fittingSize];
	}
	// otherwise, the scroller width is the smallest useful size
	return NSMakeSize([NSScroller scrollerWidthForControlSize:NSControlSizeRegular
		scrollerStyle:[self scrollerStyle]],
		[NSScroller scrollerWidthForControlSize:NSControlSizeRegular
		scrollerStyle:[self scrollerStyle]]);
}

@end

@implementation uiprivScrollDocumentView

- (BOOL)isFlipped
{
	return YES;
}

@end

static void scrollTile(uiScroll *s)
{
	NSView *child;
	NSRect clipRect;
	NSSize childSize;
	CGFloat w, h;
	NSRect docRect;

	if (s->child == nil)
		return;
	child = (NSView *) uiControlHandle(s->child);
	clipRect = [[s->sv contentView] bounds];
	// the child control is laid out manually; use its fitting size as its minimum size
	childSize = [child fittingSize];
	w = MAX(childSize.width, clipRect.size.width);
	h = MAX(childSize.height, clipRect.size.height);
	docRect = NSMakeRect(0, 0, w, h);
	if (!NSEqualRects([s->doc frame], docRect))
		[s->doc setFrame:docRect];
	if (!NSEqualRects([child frame], docRect))
		[child setFrame:docRect];
}

static void uiScrollDestroy(uiControl *c)
{
	uiScroll *s = uiScroll(c);

	if (s->child != NULL) {
		uiControlSetParent(s->child, NULL);
		uiDarwinControlSetSuperview(uiDarwinControl(s->child), nil);
		uiControlDestroy(s->child);
	}
	[s->sv release];
	[s->doc release];
	uiFreeControl(uiControl(s));
}

uiDarwinControlDefaultHandle(uiScroll, sv)
uiDarwinControlDefaultParent(uiScroll, sv)
uiDarwinControlDefaultSetParent(uiScroll, sv)
uiDarwinControlDefaultToplevel(uiScroll, sv)
uiDarwinControlDefaultVisible(uiScroll, sv)
uiDarwinControlDefaultShow(uiScroll, sv)
uiDarwinControlDefaultHide(uiScroll, sv)
uiDarwinControlDefaultEnabled(uiScroll, sv)
uiDarwinControlDefaultEnable(uiScroll, sv)
uiDarwinControlDefaultDisable(uiScroll, sv)

static void uiScrollSyncEnableState(uiDarwinControl *c, int enabled)
{
	uiScroll *s = uiScroll(c);

	if (uiDarwinShouldStopSyncEnableState(uiDarwinControl(s), enabled))
		return;
	if (s->child != NULL)
		uiDarwinControlSyncEnableState(uiDarwinControl(s->child), enabled);
}

uiDarwinControlDefaultSetSuperview(uiScroll, sv)

// the scroll view fills whatever space it is given, so it always hugs
static BOOL uiScrollHugsTrailingEdge(uiDarwinControl *c)
{
	return YES;
}

static BOOL uiScrollHugsBottom(uiDarwinControl *c)
{
	return YES;
}

static void uiScrollChildEdgeHuggingChanged(uiDarwinControl *c)
{
	uiScroll *s = uiScroll(c);

	[s->sv invalidateIntrinsicContentSize];
	scrollTile(s);
}

uiDarwinControlDefaultHuggingPriority(uiScroll, sv)
uiDarwinControlDefaultSetHuggingPriority(uiScroll, sv)

static void uiScrollChildVisibilityChanged(uiDarwinControl *c)
{
	uiScroll *s = uiScroll(c);

	[s->sv invalidateIntrinsicContentSize];
	scrollTile(s);
}

void uiScrollSetChild(uiScroll *s, uiControl *child)
{
	if (s->child != NULL) {
		uiDarwinControlSetHuggingPriority(uiDarwinControl(s->child), s->oldHorzHuggingPri, NSLayoutConstraintOrientationHorizontal);
		uiDarwinControlSetHuggingPriority(uiDarwinControl(s->child), s->oldVertHuggingPri, NSLayoutConstraintOrientationVertical);
		uiControlSetParent(s->child, NULL);
		uiDarwinControlSetSuperview(uiDarwinControl(s->child), nil);
	}
	s->child = child;
	if (s->child != NULL) {
		uiControlSetParent(s->child, uiControl(s));
		uiDarwinControlSetSuperview(uiDarwinControl(s->child), s->doc);
		uiDarwinControlSyncEnableState(uiDarwinControl(s->child), uiControlEnabledToUser(uiControl(s)));
		// don't hug, just in case we're a stretchy scroll container
		s->oldHorzHuggingPri = uiDarwinControlHuggingPriority(uiDarwinControl(s->child), NSLayoutConstraintOrientationHorizontal);
		s->oldVertHuggingPri = uiDarwinControlHuggingPriority(uiDarwinControl(s->child), NSLayoutConstraintOrientationVertical);
		uiDarwinControlSetHuggingPriority(uiDarwinControl(s->child), NSLayoutPriorityDefaultLow, NSLayoutConstraintOrientationHorizontal);
		uiDarwinControlSetHuggingPriority(uiDarwinControl(s->child), NSLayoutPriorityDefaultLow, NSLayoutConstraintOrientationVertical);
	}
	[s->sv invalidateIntrinsicContentSize];
	scrollTile(s);
}

uiScroll *uiNewScroll(void)
{
	uiScroll *s;

	uiDarwinNewControl(uiScroll, s);

	s->sv = [[uiprivScrollView alloc] initWithScroll:s];
	[s->sv setHasHorizontalScroller:YES];
	[s->sv setHasVerticalScroller:YES];
	[s->sv setAutohidesScrollers:YES];
	[s->sv setBorderType:NSNoBorder];
	[s->sv setDrawsBackground:YES];

	s->doc = [[uiprivScrollDocumentView alloc] initWithFrame:NSZeroRect];
	// we size the document view manually, so disable the autoresizing-mask constraints that
	// AppKit would otherwise install when the document view is added to the clip view
	[s->doc setTranslatesAutoresizingMaskIntoConstraints:NO];
	[s->sv setDocumentView:s->doc];

	return s;
}
