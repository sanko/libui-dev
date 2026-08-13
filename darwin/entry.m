// 14 august 2015
#import "uipriv_darwin.h"

// Text fields for entering text have no intrinsic width; we'll use the default Interface Builder width for them.
#define textfieldWidth 96

struct uiEntry {
	uiDarwinControl c;
	NSTextField *textfield;
	void (*onChanged)(uiEntry *, void *);
	void *onChangedData;
	void (*onFilesDropped)(uiEntry *, int, char **, void *);
	void *onFilesDroppedData;
	int acceptDrops;
};

@interface uiprivNSTextField : NSTextField<NSDraggingDestination> {
	uiEntry *entry;
}
- (id)initWithFrame:(NSRect)frame uiEntry:(uiEntry *)e;
@end

@implementation uiprivNSTextField

uiDarwinDragDestinationMethods(entry)

- (id)initWithFrame:(NSRect)frame uiEntry:(uiEntry *)e
{
	self = [super initWithFrame:frame];
	if (self)
		self->entry = e;
	return self;
}

- (NSSize)intrinsicContentSize
{
	NSSize s;

	s = [super intrinsicContentSize];
	s.width = textfieldWidth;
	return s;
}

@end

// TODO does this have one on its own?
@interface uiprivNSSecureTextField : NSSecureTextField<NSDraggingDestination> {
	uiEntry *entry;
}
- (id)initWithFrame:(NSRect)frame uiEntry:(uiEntry *)e;
@end

@implementation uiprivNSSecureTextField

uiDarwinDragDestinationMethods(entry)

- (id)initWithFrame:(NSRect)frame uiEntry:(uiEntry *)e
{
	self = [super initWithFrame:frame];
	if (self)
		self->entry = e;
	return self;
}

- (NSSize)intrinsicContentSize
{
	NSSize s;

	s = [super intrinsicContentSize];
	s.width = textfieldWidth;
	return s;
}

@end

// TODO does this have one on its own?
@interface uiprivNSSearchField : NSSearchField<NSDraggingDestination> {
	uiEntry *entry;
}
- (id)initWithFrame:(NSRect)frame uiEntry:(uiEntry *)e;
@end

@implementation uiprivNSSearchField

uiDarwinDragDestinationMethods(entry)

- (id)initWithFrame:(NSRect)frame uiEntry:(uiEntry *)e
{
	self = [super initWithFrame:frame];
	if (self)
		self->entry = e;
	return self;
}

- (NSSize)intrinsicContentSize
{
	NSSize s;

	s = [super intrinsicContentSize];
	s.width = textfieldWidth;
	return s;
}

@end

static BOOL isSearchField(NSTextField *tf)
{
	return [tf isKindOfClass:[NSSearchField class]];
}

@interface uiprivEntryDelegate : NSObject<NSTextFieldDelegate> {
	uiEntry *entry;
}
- (id)initWithEntry:(uiEntry *)e;
- (void)controlTextDidChange:(NSNotification *)notification;
- (IBAction)onChanged:(id)sender;
@end

@implementation uiprivEntryDelegate

- (id)initWithEntry:(uiEntry *)e
{
	self = [super init];
	if (self)
		self->entry = e;
	return self;
}

- (void)controlTextDidChange:(NSNotification *)notification
{
	[self onChanged:[notification object]];
}

- (IBAction)onChanged:(id)sender
{
	uiEntry *e = self->entry;;

	(*(e->onChanged))(e, e->onChangedData);
}

@end

uiDarwinControlAllDefaultsExceptDestroy(uiEntry, textfield)

static void uiEntryDestroy(uiControl *c)
{
	uiEntry *e = uiEntry(c);
	uiprivEntryDelegate *delegate;

	if (isSearchField(e->textfield)) {
		delegate = [e->textfield target];
		[e->textfield setTarget:nil];
	} else {
		delegate = [e->textfield delegate];
		[e->textfield setDelegate:nil];
	}
	[delegate release];

	[e->textfield release];
	uiFreeControl(uiControl(e));
}

char *uiEntryText(uiEntry *e)
{
	return uiDarwinNSStringToText([e->textfield stringValue]);
}

void uiEntrySetText(uiEntry *e, const char *text)
{
	[e->textfield setStringValue:uiprivToNSString(text)];
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
	return [e->textfield isEditable] == NO;
}

void uiEntrySetReadOnly(uiEntry *e, int readonly)
{
	BOOL editable;

	editable = YES;
	if (readonly)
		editable = NO;
	[e->textfield setEditable:editable];
}

char *uiEntryPlaceholder(uiEntry *e)
{
	NSString *text = [(NSTextFieldCell *)e->textfield.cell placeholderString];
	if (!text)
		return uiDarwinNSStringToText(@"");
	return uiDarwinNSStringToText(text);
}

void uiEntrySetPlaceholder(uiEntry *e, const char *text)
{
	[(NSTextFieldCell *)e->textfield.cell setPlaceholderString:uiprivToNSString(text)];
}

static void defaultOnChanged(uiEntry *e, void *data)
{
	// do nothing
}

NSTextField *uiprivNewEditableTextField(void)
{
	NSTextField *tf;

	tf = [[uiprivNSTextField alloc] initWithFrame:NSZeroRect];
	uiprivNSTextFieldSetStyleEntry(tf);
	return tf;
}

static uiEntry *finishNewEntry(Class class)
{
	uiEntry *e;
	uiprivEntryDelegate *delegate;

	uiDarwinNewControl(uiEntry, e);

	e->textfield = [[class alloc] initWithFrame:NSZeroRect uiEntry:e];
	uiprivNSTextFieldSetStyleEntry(e->textfield);

	delegate = [[uiprivEntryDelegate alloc] initWithEntry:e];
	if (isSearchField(e->textfield)) {
		[e->textfield setTarget:delegate];
		[e->textfield setAction:@selector(onChanged:)];
	} else {
		[e->textfield setDelegate:delegate];
	}

	uiEntryOnChanged(e, defaultOnChanged, NULL);
	uiEntryOnFilesDropped(e, defaultOnFilesDropped, NULL);
	uiEntrySetAcceptDrops(e, 0);

	return e;
}

uiEntry *uiNewEntry(void)
{
	return finishNewEntry([uiprivNSTextField class]);
}

uiEntry *uiNewPasswordEntry(void)
{
	return finishNewEntry([uiprivNSSecureTextField class]);
}

uiEntry *uiNewSearchEntry(void)
{
	uiEntry *e;
	NSSearchField *s;

	e = finishNewEntry([uiprivNSSearchField class]);
	s = (NSSearchField *) (e->textfield);
	uiprivNSTextFieldSetStyleSearchEntry(s);
	return e;
}
