#include "unit.h"

#include <stdio.h>

// ui_version.h is the single source of truth for the version; these checks
// verify the derived macros are consistent with the three numeric ones.
// LIBUI_VERSION_STRING is checked against LIBUI_VERSION_STRING_BASE only: in
// builds that append the git commit hash, LIBUI_VERSION_STRING is "x.y.z-hash".
static void versionMacros(void **state)
{
	int major, minor, patch;
	char versionString[32];

	assert_int_equal(sscanf(LIBUI_VERSION_STRING_BASE, "%d.%d.%d",
		&major, &minor, &patch), 3);
	assert_int_equal(major, LIBUI_VERSION_MAJOR);
	assert_int_equal(minor, LIBUI_VERSION_MINOR);
	assert_int_equal(patch, LIBUI_VERSION_PATCH);
	assert_int_equal(LIBUI_VERSION_INT, LIBUI_VERSION_INT_HELPER(major, minor, patch));
	snprintf(versionString, sizeof versionString, "%d.%d.%d",
		LIBUI_VERSION_MAJOR, LIBUI_VERSION_MINOR, LIBUI_VERSION_PATCH);
	assert_string_equal(LIBUI_VERSION_STRING_BASE, versionString);
}

static void initUninit(void **state)
{
	uiInitOptions o = {0};

	assert_null(uiInit(&o));
	uiUninit();
}

static void initUninitTwice(void **state)
{
	uiInitOptions o = {0};

	assert_null(uiInit(&o));
	uiUninit();

	assert_null(uiInit(&o));
	uiUninit();
}

static void setAppMetadataThenInit(void **state)
{
	uiInitOptions o = {0};

	uiSetAppMetadata("test", "1.2.3", "test.test");
	assert_null(uiInit(&o));
	uiUninit();
}

static void setAppMetadataNullsThenInit(void **state)
{
	uiInitOptions o = {0};

	uiSetAppMetadata(NULL, NULL, NULL);
	assert_null(uiInit(&o));
	uiUninit();
}

#if !defined(_WIN32) && !defined(__APPLE__)
static void mainStepsResetAfterQuit(void **state)
{
	uiInitOptions o = {0};
	int i;

	assert_null(uiInit(&o));
	uiMainSteps();
	uiQuit();
	for (i = 0; i < 100; i++)
		if (!uiMainStep(0))
			break;
	assert_int_in_range(i, 0, 99);

	uiMainSteps();
	assert_true(uiMainStep(0));
	uiUninit();
}
#endif

#if !defined(__APPLE__)
struct timerState {
	int count;
	int stopAfter;
};

static int repeatTimer(void *data)
{
	struct timerState *state = data;

	state->count++;
	if (state->count >= state->stopAfter) {
		uiQuit();
		return 0;
	}
	return 1;
}

static void timerOneShot(void **state)
{
	uiInitOptions o = {0};
	struct timerState timerState = { 0, 1 };

	assert_null(uiInit(&o));
	uiTimer(1, repeatTimer, &timerState);
	uiMain();
	assert_int_equal(timerState.count, 1);
	uiUninit();
}

static void timerRepeatThenStop(void **state)
{
	uiInitOptions o = {0};
	struct timerState timerState = { 0, 3 };

	assert_null(uiInit(&o));
	uiTimer(1, repeatTimer, &timerState);
	uiMain();
	assert_int_equal(timerState.count, 3);
	uiUninit();
}
#endif

int initRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(versionMacros),
		cmocka_unit_test(initUninit),
		cmocka_unit_test(initUninitTwice),
		cmocka_unit_test(setAppMetadataThenInit),
		cmocka_unit_test(setAppMetadataNullsThenInit),
#if !defined(_WIN32) && !defined(__APPLE__)
		cmocka_unit_test(mainStepsResetAfterQuit),
#endif
#if !defined(__APPLE__)
		cmocka_unit_test(timerOneShot),
		cmocka_unit_test(timerRepeatThenStop),
#endif
	};

	return cmocka_run_group_tests_name("uiInit", tests, NULL, NULL);
}
