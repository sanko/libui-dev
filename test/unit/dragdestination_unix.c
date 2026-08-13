// Unix (GTK3) drag and drop unit tests.
//
// These synthesize an in-process drag-and-drop session and drive the
// destination widget's drag-motion / drag-drop / drag-data-received /
// drag-leave signal handlers that uiControlRegisterDragDestination()
// connects. A real GdkDragContext is created with
// gtk_drag_begin_with_coordinates() so that gtk_drag_get_data() can route
// the data request back to the source widget's drag-data-get handler,
// exercising the full enter -> move -> data -> drop path.
//
// This test is only built on Unix (Linux).

#include "unit.h"

#include <gtk/gtk.h>

// drag source state: what data the drag-data-get handler provides
struct unixDragSource {
	const char *text;
	char **uris;
};

static void unixDragSourceDataGet(GtkWidget *widget, GdkDragContext *context,
	GtkSelectionData *selection_data, guint info, guint time, gpointer data)
{
	struct unixDragSource *src = (struct unixDragSource *) data;

	switch (info) {
		case uiDragTypeText:
			gtk_selection_data_set_text(selection_data, src->text, -1);
			break;
		case uiDragTypeURIs:
			gtk_selection_data_set_uris(selection_data, src->uris);
			break;
	}
}

// shared drag callback state
struct unixDragState {
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

static void unixDragStateFree(struct unixDragState *s)
{
	free(s->gotText);
	if (s->gotFiles != NULL) {
		int i;

		for (i = 0; i < s->gotFileCount; i++)
			free(s->gotFiles[i]);
		free(s->gotFiles);
	}
}

static char *unixDragDup(const char *str)
{
	size_t len = strlen(str) + 1;
	char *out = (char *) malloc(len);

	if (out == NULL)
		return NULL;
	memcpy(out, str, len);
	return out;
}

static uiDragOperation unixDragOnEnter(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	struct unixDragState *s = (struct unixDragState *) data;

	s->enters++;
	uiDragContextPosition(dc, &s->gotX, &s->gotY);
	s->gotTypes = uiDragContextDragTypes(dc);
	s->gotOps = uiDragContextDragOperations(dc);
	return s->enterResult;
}

static uiDragOperation unixDragOnMove(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	struct unixDragState *s = (struct unixDragState *) data;

	s->moves++;
	return s->moveResult;
}

static void unixDragOnExit(uiDragDestination *dd, void *data)
{
	struct unixDragState *s = (struct unixDragState *) data;

	s->exits++;
}

static int unixDragOnDrop(uiDragDestination *dd, uiDragContext *dc, void *data)
{
	struct unixDragState *s = (struct unixDragState *) data;
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
			s->gotText = unixDragDup(d->data.text);
			if (s->gotText == NULL)
				s->fetchFailed = 1;
		} else if (d->type == uiDragTypeURIs) {
			s->gotFileCount = d->data.URIs.numURIs;
			s->gotFiles = (char **) calloc((size_t) s->gotFileCount, sizeof(char *));
			if (s->gotFiles == NULL)
				s->fetchFailed = 1;
			for (i = 0; i < s->gotFileCount && s->gotFiles != NULL; i++) {
				s->gotFiles[i] = unixDragDup(d->data.URIs.URIs[i]);
				if (s->gotFiles[i] == NULL)
					s->fetchFailed = 1;
			}
		}
		uiFreeDragData(d);
	}
	return s->dropResult;
}

static void unixDragRegister(uiControl *c, struct unixDragState *s)
{
	uiDragDestination *dd;

	dd = uiNewDragDestination();
	uiDragDestinationSetAcceptTypes(dd, uiDragTypeText | uiDragTypeURIs);
	uiDragDestinationOnEnter(dd, unixDragOnEnter, s);
	uiDragDestinationOnMove(dd, unixDragOnMove, s);
	uiDragDestinationOnExit(dd, unixDragOnExit, s);
	uiDragDestinationOnDrop(dd, unixDragOnDrop, s);
	uiControlRegisterDragDestination(uiControl(c), dd);

	s->dd = dd;
}

// starts a drag with the given targets from a fresh source window and
// returns the drag context
static GdkDragContext *unixDragBegin(const char *const *targetNames, int *targetInfos, int nTargets)
{
	GtkWidget *sourceWin;
	GtkWidget *sourceLabel;
	GtkTargetEntry *entries;
	int i;
	GtkTargetList *targets;
	GdkDragContext *context;

	sourceWin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	sourceLabel = gtk_label_new("drag source");
	gtk_container_add(GTK_CONTAINER(sourceWin), sourceLabel);
	gtk_widget_show_all(sourceWin);

	entries = g_new(GtkTargetEntry, nTargets);
	for (i = 0; i < nTargets; i++) {
		entries[i].target = (gchar *) targetNames[i];
		entries[i].flags = GTK_TARGET_OTHER_APP;
		entries[i].info = targetInfos[i];
	}
	targets = gtk_target_list_new(entries, nTargets);
	g_free(entries);

	context = gtk_drag_begin_with_coordinates(sourceLabel, targets,
		GDK_ACTION_COPY | GDK_ACTION_LINK | GDK_ACTION_MOVE, 1, NULL, 0, 0);
	return context;
}

#define uiLabelPtrFromState(s) uiControlPtrFromState(uiLabel, s)

static void unixDragTextDrop(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct unixDragState s;
	struct unixDragSource src;
	GdkDragContext *context;
	GtkWidget *target;
	const char *targetNames[] = { "text/plain;charset=utf-8" };
	int targetInfos[] = { uiDragTypeText };
	gboolean handled;

	*c = uiNewLabel("drag destination");
	target = (GtkWidget *) uiControlHandle(uiControl(*c));

	memset(&s, 0, sizeof(s));
	unixDragRegister(uiControl(*c), &s);

	memset(&src, 0, sizeof(src));
	src.text = "hello drag";

	context = unixDragBegin(targetNames, targetInfos, 1);
	assert_non_null(context);

	s.enterResult = uiDragOperationCopy;
	s.moveResult = uiDragOperationMove;
	s.dropResult = 1;
	s.fetchType = uiDragTypeText;

	g_signal_connect(target, "drag-data-get", G_CALLBACK(unixDragSourceDataGet), &src);
	g_signal_emit_by_name(target, "drag-motion", context, 12, 34, GDK_CURRENT_TIME, &handled);
	assert_int_equal(s.enters, 1);
	assert_int_equal(s.exits, 0);
	assert_int_equal(s.gotX, 12);
	assert_int_equal(s.gotY, 34);
	assert_int_equal(s.gotTypes, uiDragTypeText);
	assert_int_equal(s.gotOps, uiDragOperationCopy | uiDragOperationLink | uiDragOperationMove);

	g_signal_emit_by_name(target, "drag-motion", context, 15, 34, GDK_CURRENT_TIME, &handled);
	assert_int_equal(s.moves, 1);

	g_signal_emit_by_name(target, "drag-drop", context, 15, 34, GDK_CURRENT_TIME, &handled);
	assert_true(handled);
	assert_int_equal(s.drops, 1);
	assert_false(s.fetchFailed);
	assert_string_equal(s.gotText, "hello drag");
	assert_int_equal(s.exits, 0);

	g_signal_handler_disconnect(target, g_signal_lookup("drag-data-get", G_OBJECT_TYPE(target)));
	unixDragStateFree(&s);
}

static void unixDragUriDrop(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct unixDragState s;
	struct unixDragSource src;
	GdkDragContext *context;
	GtkWidget *target;
	const char *targetNames[] = { "text/uri-list" };
	int targetInfos[] = { uiDragTypeURIs };
	char *uris[] = { "file:///tmp/a.txt", "file:///tmp/b%20file.txt", NULL };
	gboolean handled;

	*c = uiNewLabel("drag destination");
	target = (GtkWidget *) uiControlHandle(uiControl(*c));

	memset(&s, 0, sizeof(s));
	unixDragRegister(uiControl(*c), &s);

	memset(&src, 0, sizeof(src));
	src.uris = uris;

	context = unixDragBegin(targetNames, targetInfos, 1);
	assert_non_null(context);

	s.enterResult = uiDragOperationCopy;
	s.dropResult = 1;
	s.fetchType = uiDragTypeURIs;

	g_signal_connect(target, "drag-data-get", G_CALLBACK(unixDragSourceDataGet), &src);
	g_signal_emit_by_name(target, "drag-motion", context, 0, 0, GDK_CURRENT_TIME, &handled);
	assert_int_equal(s.enters, 1);
	assert_int_equal(s.gotTypes, uiDragTypeURIs);

	g_signal_emit_by_name(target, "drag-drop", context, 0, 0, GDK_CURRENT_TIME, &handled);
	assert_true(handled);
	assert_int_equal(s.drops, 1);
	assert_false(s.fetchFailed);
	assert_int_equal(s.gotFileCount, 2);
	assert_string_equal(s.gotFiles[0], "/tmp/a.txt");
	assert_string_equal(s.gotFiles[1], "/tmp/b file.txt");

	g_signal_handler_disconnect(target, g_signal_lookup("drag-data-get", G_OBJECT_TYPE(target)));
	unixDragStateFree(&s);
}

static void unixDragExit(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct unixDragState s;
	struct unixDragSource src;
	GdkDragContext *context;
	GtkWidget *target;
	const char *targetNames[] = { "text/plain;charset=utf-8" };
	int targetInfos[] = { uiDragTypeText };
	gboolean handled;

	*c = uiNewLabel("drag destination");
	target = (GtkWidget *) uiControlHandle(uiControl(*c));

	memset(&s, 0, sizeof(s));
	unixDragRegister(uiControl(*c), &s);

	memset(&src, 0, sizeof(src));
	src.text = "hello drag";

	context = unixDragBegin(targetNames, targetInfos, 1);
	assert_non_null(context);

	s.enterResult = uiDragOperationCopy;
	s.dropResult = 1;

	g_signal_connect(target, "drag-data-get", G_CALLBACK(unixDragSourceDataGet), &src);
	g_signal_emit_by_name(target, "drag-motion", context, 0, 0, GDK_CURRENT_TIME, &handled);
	assert_int_equal(s.enters, 1);

	g_signal_emit_by_name(target, "drag-leave", context, GDK_CURRENT_TIME);
	assert_int_equal(s.exits, 1);

	g_signal_handler_disconnect(target, g_signal_lookup("drag-data-get", G_OBJECT_TYPE(target)));
	unixDragStateFree(&s);
}

static void unixDragDropRejected(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	struct unixDragState s;
	struct unixDragSource src;
	GdkDragContext *context;
	GtkWidget *target;
	const char *targetNames[] = { "text/plain;charset=utf-8" };
	int targetInfos[] = { uiDragTypeText };
	gboolean handled;

	*c = uiNewLabel("drag destination");
	target = (GtkWidget *) uiControlHandle(uiControl(*c));

	memset(&s, 0, sizeof(s));
	unixDragRegister(uiControl(*c), &s);

	memset(&src, 0, sizeof(src));
	src.text = "hello drag";

	context = unixDragBegin(targetNames, targetInfos, 1);
	assert_non_null(context);

	s.enterResult = uiDragOperationCopy;
	s.dropResult = 0;
	s.fetchType = uiDragTypeText;

	g_signal_connect(target, "drag-data-get", G_CALLBACK(unixDragSourceDataGet), &src);
	g_signal_emit_by_name(target, "drag-motion", context, 0, 0, GDK_CURRENT_TIME, &handled);
	g_signal_emit_by_name(target, "drag-drop", context, 0, 0, GDK_CURRENT_TIME, &handled);
	assert_false(handled);
	assert_int_equal(s.drops, 1);
	assert_false(s.fetchFailed);
	assert_string_equal(s.gotText, "hello drag");

	g_signal_handler_disconnect(target, g_signal_lookup("drag-data-get", G_OBJECT_TYPE(target)));
	unixDragStateFree(&s);
}

#define unixDragUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int platformDragDropRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		unixDragUnitTest(unixDragTextDrop),
		unixDragUnitTest(unixDragUriDrop),
		unixDragUnitTest(unixDragExit),
		unixDragUnitTest(unixDragDropRejected),
	};

	return cmocka_run_group_tests_name("dragdrop-unix", tests,
		unitTestsSetup, unitTestsTeardown);
}
