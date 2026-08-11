#include "unit.h"

#define uiWindowFromState(s) (((struct state *)*(s))->w)

static void windowNew(void **state)
{
	uiInitOptions o = {0};
	uiWindow *w;

	assert_null(uiInit(&o));
	w = uiNewWindow(UNIT_TEST_WINDOW_TITLE, UNIT_TEST_WINDOW_WIDTH, UNIT_TEST_WINDOW_HEIGHT, 0);
	uiWindowOnClosing(w, unitWindowOnClosingQuit, NULL);
	uiControlShow(uiControl(w));

	//uiMain();
	uiMainSteps();
	uiMainStep(1);
	uiControlDestroy(uiControl(w));
	uiUninit();
}

static void windowDefaultMargined(void **state)
{
	uiWindow *w = uiWindowFromState(state);

	assert_int_equal(uiWindowMargined(w), 0);
}

static void windowMargined(void **state)
{
	uiWindow *w = uiWindowFromState(state);

	uiWindowSetMargined(w, 1);
	assert_int_equal(uiWindowMargined(w), 1);

	uiWindowSetMargined(w, 0);
	assert_int_equal(uiWindowMargined(w), 0);
}

static void windowDefaultTitle(void **state)
{
	uiWindow *w = uiWindowFromState(state);
	char *rv;

	rv = uiWindowTitle(w);
	assert_string_equal(rv, UNIT_TEST_WINDOW_TITLE);
	uiFreeText(rv);
}

static void windowSetTitle(void **state)
{
	uiWindow *w = uiWindowFromState(state);
	const char *text = "SetTitle";
	char *rv;

	uiWindowSetTitle(w, text);
	rv = uiWindowTitle(w);
	assert_string_equal(rv, text);
	uiFreeText(rv);
}

static void windowDefaultContentSize(void **state)
{
	uiWindow *w = uiWindowFromState(state);
	int width, height;

	uiWindowContentSize(w, &width, &height);
	assert_int_equal(width, UNIT_TEST_WINDOW_WIDTH);
	assert_int_equal(height, UNIT_TEST_WINDOW_HEIGHT);
}

static void windowSetPosition(void **state)
{
	uiWindow *w = uiWindowFromState(state);
	int x, y;

	uiWindowSetPosition(w, 0, 0);
	uiWindowPosition(w, &x, &y);
	assert_int_equal(x, 0);
	assert_int_equal(y, 0);

	uiWindowSetPosition(w, 1, 1);
	uiWindowPosition(w, &x, &y);
	assert_int_equal(x, 1);
	assert_int_equal(y, 1);
}

static void windowSetContentSize(void **state)
{
	uiWindow *w = uiWindowFromState(state);
	int width, height;

	uiWindowSetContentSize(w, UNIT_TEST_WINDOW_WIDTH + 10, UNIT_TEST_WINDOW_HEIGHT + 10);

	uiWindowContentSize(w, &width, &height);
	assert_int_equal(width, UNIT_TEST_WINDOW_WIDTH + 10);
	assert_int_equal(height, UNIT_TEST_WINDOW_HEIGHT + 10);
}

static void windowMarginedSetContentSize(void **state)
{
	uiWindow *w = uiWindowFromState(state);
	int width, height;

	uiWindowSetMargined(w, 0);
	uiWindowSetContentSize(w, UNIT_TEST_WINDOW_WIDTH + 10, UNIT_TEST_WINDOW_HEIGHT + 10);

	uiWindowContentSize(w, &width, &height);
	assert_int_equal(width, UNIT_TEST_WINDOW_WIDTH + 10);
	assert_int_equal(height, UNIT_TEST_WINDOW_HEIGHT + 10);

	uiWindowSetMargined(w, 1);
	uiWindowSetContentSize(w, UNIT_TEST_WINDOW_WIDTH + 10, UNIT_TEST_WINDOW_HEIGHT + 10);

	uiWindowContentSize(w, &width, &height);
	assert_int_equal(width, UNIT_TEST_WINDOW_WIDTH + 10);
	assert_int_equal(height, UNIT_TEST_WINDOW_HEIGHT + 10);
}

void onContentSizeChangedNoCall(uiWindow *w, void *data)
{
	function_called();
}

static void windowSetContentSizeNoCallback(void **state)
{
	uiWindow *w = uiWindowFromState(state);

	uiWindowOnContentSizeChanged(w, onContentSizeChangedNoCall, NULL);
	uiWindowSetContentSize(w, UNIT_TEST_WINDOW_WIDTH + 10, UNIT_TEST_WINDOW_HEIGHT + 10);
	uiWindowSetContentSize(w, UNIT_TEST_WINDOW_WIDTH + 20, UNIT_TEST_WINDOW_HEIGHT + 20);
}

void onPositionChangedCallback(uiWindow *w, void *data)
{
	function_called();
}

static void windowSetPositionNoCallback(void **state)
{
	uiWindow *w = uiWindowFromState(state);

	uiWindowOnPositionChanged(w, onPositionChangedCallback, NULL);
	uiWindowSetPosition(w, 0, 0);
	uiWindowSetPosition(w, 1, 1);
}

static const unsigned char windowTestIcon[] = {
	// ICO header: reserved, type (1 = icon), image count (1)
	0x00, 0x00, 0x01, 0x00, 0x01, 0x00,
	// ICONDIRENTRY
	0x01, 0x01, 0x00, 0x00, 0x01, 0x00, 0x20, 0x00,
	0x30, 0x00, 0x00, 0x00, 0x16, 0x00, 0x00, 0x00,
	// BITMAPINFOHEADER (1x1, 32bpp)
	0x28, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00,
	0x01, 0x00,
	0x20, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	// XOR data: 1 pixel BGRA (opaque red)
	0x00, 0x00, 0xFF, 0xFF,
	// AND data: 1 row, 1bpp, padded to a DWORD
	0x00, 0x00, 0x00, 0x00,
};

static void windowSetIcon(void **state)
{
	uiWindow *w = uiWindowFromState(state);

	uiWindowSetIcon(w, windowTestIcon, sizeof(windowTestIcon));
	uiWindowSetIcon(w, windowTestIcon, sizeof(windowTestIcon));
}

#define windowUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int windowRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(windowNew),
		windowUnitTest(windowDefaultMargined),
		windowUnitTest(windowMargined),
		windowUnitTest(windowDefaultTitle),
		windowUnitTest(windowDefaultContentSize),
		windowUnitTest(windowSetTitle),
		windowUnitTest(windowSetPosition),
		windowUnitTest(windowSetContentSize),
		windowUnitTest(windowMarginedSetContentSize),
		windowUnitTest(windowSetContentSizeNoCallback),
		windowUnitTest(windowSetPositionNoCallback),
		windowUnitTest(windowSetIcon),
	};

	return cmocka_run_group_tests_name("uiWindow", tests, unitTestsSetup, unitTestsTeardown);
}
