// Darwin (AppKit) drag and drop unit tests.
//
// These synthesize an in-process drag-and-drop session by driving the
// NSDraggingDestination methods of a label's view directly, using a mock
// NSDraggingInfo whose pasteboard carries real NSPasteboard data. This
// exercises the full enter -> move -> prepare -> drop -> exit path against
// the drag destination registered by uiControlRegisterDragDestination()
// without going through the real drag-session machinery.
//
// This test is only built on macOS.

#import <Cocoa/Cocoa.h>

#include "unit.h"

// The drag destination callbacks are attached to each control's view in the
// darwin backend (uiprivDragDestinationMethods in ui_darwin.h, plus the
// NSView category in dragdestination.m). Redeclare the selectors so the
// compiler knows the methods exist on NSView.
@interface NSView (uiprivUnitTestDragDestination)
- (NSDragOperation)draggingEntered:(id<NSDraggingInfo>)sender;
- (NSDragOperation)draggingUpdated:(id<NSDraggingInfo>)sender;
- (void)draggingExited:(id<NSDraggingInfo>)sender;
- (BOOL)prepareForDragOperation:(id<NSDraggingInfo>)sender;
- (BOOL)performDragOperation:(id<NSDraggingInfo>)sender;
@end

// mock NSDraggingInfo: a pasteboard plus the geometry and operation mask a
// real drag session would provide
@interface MockDraggingInfo : NSObject {
	NSPasteboard *m_pboard;
	NSPoint m_location;
	NSDragOperation m_sourceOperationMask;
}
- (instancetype)initWithPasteboard:(NSPasteboard *)pboard;
- (void)setLocationX:(CGFloat)x y:(CGFloat)y;
- (void)setSourceOperationMask:(NSDragOperation)mask;
@end

@implementation MockDraggingInfo

- (instancetype)initWithPasteboard:(NSPasteboard *)pboard
{
	self = [super init];
	if (self) {
		m_pboard = [pboard retain];
		m_location = NSZeroPoint;
		m_sourceOperationMask = NSDragOperationNone;
	}
	return self;
}

- (void)dealloc
{
	[m_pboard release];
	[super dealloc];
}

- (NSPasteboard *)draggingPasteboard
{
	return m_pboard;
}

- (NSPoint)draggingLocation
{
	return m_location;
}

- (NSDragOperation)draggingSourceOperationMask
{
	return m_sourceOperationMask;
}

- (void)setLocationX:(CGFloat)x y:(CGFloat)y
{
	m_location = NSMakePoint(x, y);
}

- (void)setSourceOperationMask:(NSDragOperation)mask
{
	m_sourceOperationMask = mask;
}

@end

// shared drag callback state
struct darwinDragState {
	uiDragDestination *dd;
	int enters;
	int moves;
	int exits;
	int drops;
	uiDragOperation enterResult;
	uiDragOperation moveResult;
	int dropResult;
	// captured inside the enter callback
	int gotX;
	int gotY;
	int gotTypes;
	int gotOps;
	// data to fetch inside the drop callback, and where to store it
	uiDragType fetchType;
	int fetchFailed;
	char *gotText;
	int gotFileCount;
	char **gotFiles;
};

static void darwinDragStateFree(struct darwinDragState *s)
{
	free(s->gotText);
	if (s->gotFiles != NULL) {
		int i;

		for (i = 0; i < s->gotFileCount; i++)
			free(s->gotFiles[i]);
		free(s->gotFiles);
	}
}

static char *darwinDragDup(const char *str)
{
	size_t len = strlen(str) + 1;
	char *out = (char *) malloc(len);

	if (out == NULL)
		return NULL;
	memcpy(out, str, len);
	return out;
}

static uiDragOperation darwinDragOnEnter(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	struct darwinDragState *s = (struct darwinDragState *) data;

	s->enters++;
	uiDragContextPosition(dc, &s->gotX, &s->gotY);
	s->gotTypes = uiDragContextDragTypes(dc);
	s->gotOps = uiDragContextDragOperations(dc);
	return s->enterResult;
}

static uiDragOperation darwinDragOnMove(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	struct darwinDragState *s = (struct darwinDragState *) data;

	s->moves++;
	return s->moveResult;
}

static void darwinDragOnExit(uiDragDestination *dd, void *data)
{
	struct darwinDragState *s = (struct darwinDragState *) data;

	s->exits++;
}

static int darwinDragOnDrop(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	struct darwinDragState *s = (struct darwinDragState *) data;
	uiDragData *d;
	int i;

	s->drops++;
	if (s->fetchType != 0) {
		d = uiDragContextDragData(dc, s->fetchType);
		if (d == NULL) {
			s->fetchFailed = 1;
			return 0;
		}
		if (d->type == uiDragTypeText) {
			s->gotText = darwinDragDup(d->data.text);
			if (s->gotText == NULL)
				s->fetchFailed = 1;
		} else if (d->type == uiDragTypeURIs) {
			s->gotFileCount = d->data.URIs.numURIs;
			s->gotFiles = (char **) calloc((size_t) s->gotFileCount, sizeof(char *));
			if (s->gotFiles == NULL)
				s->fetchFailed = 1;
			for (i = 0; i < s->gotFileCount && s->gotFiles != NULL; i++) {
				s->gotFiles[i] = darwinDragDup(d->data.URIs.URIs[i]);
				if (s->gotFiles[i] == NULL)
					s->fetchFailed = 1;
			}
		}
		uiFreeDragData(d);
	}
	return s->dropResult;
}

static void darwinDragRegister(uiControl *c, struct darwinDragState *s)
{
	uiDragDestination *dd;

	dd = uiNewDragDestination();
	uiDragDestinationSetAcceptTypes(dd, uiDragTypeText | uiDragTypeURIs);
	uiDragDestinationOnEnter(dd, darwinDragOnEnter, s);
	uiDragDestinationOnMove(dd, darwinDragOnMove, s);
	uiDragDestinationOnExit(dd, darwinDragOnExit, s);
	uiDragDestinationOnDrop(dd, darwinDragOnDrop, s);
	uiControlRegisterDragDestination(uiControl(c), dd);

	s->dd = dd;
}

#define uiLabelPtrFromState(s) uiControlPtrFromState(uiLabel, s)

static void darwinDragTextDrop(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct darwinDragState s;
	NSView *view;
	NSPasteboard *pboard;
	MockDraggingInfo *info;
	NSDragOperation op;
	BOOL ok;

	@autoreleasepool {
		*c = uiNewLabel("drag destination");
		view = (NSView *) uiControlHandle(uiControl(*c));

		memset(&s, 0, sizeof(s));
		darwinDragRegister(uiControl(*c), &s);

		pboard = [NSPasteboard pasteboardWithUniqueName];
		[pboard declareTypes:@[NSPasteboardTypeString] owner:nil];
		[pboard setString:@"hello drag" forType:NSPasteboardTypeString];

		info = [[MockDraggingInfo alloc] initWithPasteboard:pboard];
		[info setLocationX:42 y:37];
		[info setSourceOperationMask:NSDragOperationCopy | NSDragOperationLink | NSDragOperationMove];

		s.enterResult = uiDragOperationCopy;
		s.moveResult = uiDragOperationMove;
		s.dropResult = 1;
		s.fetchType = uiDragTypeText;

		op = [view draggingEntered:info];
		assert_int_equal((int) op, (int) NSDragOperationCopy);
		assert_int_equal(s.enters, 1);
		assert_int_equal(s.exits, 0);
		assert_int_equal(s.gotX, 42);
		// uiDragContextPosition() flips to a top-left-origin coordinate space
		assert_int_equal(s.gotY, (int) (view.frame.size.height - 37));
		assert_int_equal(s.gotTypes, uiDragTypeText);
		assert_int_equal(s.gotOps, uiDragOperationCopy | uiDragOperationLink | uiDragOperationMove);
		assert_int_equal(uiDragDestinationLastDragOperation(s.dd), uiDragOperationCopy);

		op = [view draggingUpdated:info];
		assert_int_equal((int) op, (int) NSDragOperationMove);
		assert_int_equal(s.moves, 1);

		ok = [view prepareForDragOperation:info];
		assert_true(ok);

		ok = [view performDragOperation:info];
		assert_true(ok);
		assert_int_equal(s.drops, 1);
		assert_false(s.fetchFailed);
		assert_string_equal(s.gotText, "hello drag");
		assert_int_equal(s.exits, 0);

		[view draggingExited:info];
		assert_int_equal(s.exits, 1);

		[info release];
		[pboard release];
		darwinDragStateFree(&s);
	}
}

static void darwinDragUriDrop(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct darwinDragState s;
	NSView *view;
	NSPasteboard *pboard;
	MockDraggingInfo *info;
	BOOL ok;

	@autoreleasepool {
		*c = uiNewLabel("drag destination");
		view = (NSView *) uiControlHandle(uiControl(*c));

		memset(&s, 0, sizeof(s));
		darwinDragRegister(uiControl(*c), &s);

		pboard = [NSPasteboard pasteboardWithUniqueName];
		[pboard writeObjects:@[[NSURL fileURLWithPath:@"/tmp/dropped-a.txt"],
			[NSURL fileURLWithPath:@"/tmp/dropped-b file.txt"]]];

		info = [[MockDraggingInfo alloc] initWithPasteboard:pboard];
		[info setLocationX:0 y:0];
		[info setSourceOperationMask:NSDragOperationCopy];

		s.enterResult = uiDragOperationCopy;
		s.dropResult = 1;
		s.fetchType = uiDragTypeURIs;

		(void) [view draggingEntered:info];
		assert_int_equal(s.enters, 1);
		assert_int_equal(s.gotTypes, uiDragTypeURIs);

		ok = [view performDragOperation:info];
		assert_true(ok);
		assert_int_equal(s.drops, 1);
		assert_false(s.fetchFailed);
		assert_int_equal(s.gotFileCount, 2);
		assert_string_equal(s.gotFiles[0], "/tmp/dropped-a.txt");
		assert_string_equal(s.gotFiles[1], "/tmp/dropped-b file.txt");

		[info release];
		[pboard release];
		darwinDragStateFree(&s);
	}
}

static void darwinDragRejectWrongTypes(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct darwinDragState s;
	NSView *view;
	NSPasteboard *pboard;
	MockDraggingInfo *info;
	BOOL ok;

	@autoreleasepool {
		*c = uiNewLabel("drag destination");
		view = (NSView *) uiControlHandle(uiControl(*c));

		memset(&s, 0, sizeof(s));
		darwinDragRegister(uiControl(*c), &s);

		// a pasteboard carrying nothing the destination accepts: unlike
		// Windows, the enter callback still runs (type filtering is deferred
		// to the data fetch inside the drop callback)
		pboard = [NSPasteboard pasteboardWithUniqueName];
		[pboard declareTypes:@[] owner:nil];

		info = [[MockDraggingInfo alloc] initWithPasteboard:pboard];
		[info setLocationX:0 y:0];
		[info setSourceOperationMask:NSDragOperationCopy];

		s.enterResult = uiDragOperationCopy;
		s.dropResult = 1;
		s.fetchType = uiDragTypeText;

		(void) [view draggingEntered:info];
		assert_int_equal(s.enters, 1);
		assert_int_equal(s.gotTypes, 0);

		ok = [view performDragOperation:info];
		assert_false(ok);
		assert_int_equal(s.drops, 1);
		assert_true(s.fetchFailed);

		[info release];
		[pboard release];
		darwinDragStateFree(&s);
	}
}

static void darwinDragDropRejected(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct darwinDragState s;
	NSView *view;
	NSPasteboard *pboard;
	MockDraggingInfo *info;
	BOOL ok;

	@autoreleasepool {
		*c = uiNewLabel("drag destination");
		view = (NSView *) uiControlHandle(uiControl(*c));

		memset(&s, 0, sizeof(s));
		darwinDragRegister(uiControl(*c), &s);

		pboard = [NSPasteboard pasteboardWithUniqueName];
		[pboard declareTypes:@[NSPasteboardTypeString] owner:nil];
		[pboard setString:@"hello drag" forType:NSPasteboardTypeString];

		info = [[MockDraggingInfo alloc] initWithPasteboard:pboard];
		[info setLocationX:0 y:0];
		[info setSourceOperationMask:NSDragOperationCopy];

		s.enterResult = uiDragOperationCopy;
		s.dropResult = 0;
		s.fetchType = uiDragTypeText;

		(void) [view draggingEntered:info];

		ok = [view performDragOperation:info];
		assert_false(ok);
		assert_int_equal(s.drops, 1);
		assert_false(s.fetchFailed);
		assert_string_equal(s.gotText, "hello drag");

		[info release];
		[pboard release];
		darwinDragStateFree(&s);
	}
}

#define darwinDragUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int platformDragDropRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		darwinDragUnitTest(darwinDragTextDrop),
		darwinDragUnitTest(darwinDragUriDrop),
		darwinDragUnitTest(darwinDragRejectWrongTypes),
		darwinDragUnitTest(darwinDragDropRejected),
	};

	return cmocka_run_group_tests_name("dragdrop-darwin", tests,
		unitTestsSetup, unitTestsTeardown);
}
