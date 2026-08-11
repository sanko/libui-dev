#include "unit.h"

#define uiScrollPtrFromState(s) uiControlPtrFromState(uiScroll, s)

static void scrollSetChildNoCrash(void **state)
{
	uiScroll **s = uiScrollPtrFromState(state);

	*s = uiNewScroll();
	uiScrollSetChild(*s, uiControl(uiNewLabel("Scroll content")));
}

static void scrollReplaceChildNoCrash(void **state)
{
	uiScroll **s = uiScrollPtrFromState(state);
	uiLabel *l1;
	uiLabel *l2;
	uiLabel *l3;

	*s = uiNewScroll();
	l1 = uiNewLabel("First child");
	l2 = uiNewLabel("Second child");
	l3 = uiNewLabel("Third child");
	uiScrollSetChild(*s, uiControl(l1));
	uiScrollSetChild(*s, NULL);
	uiControlDestroy(uiControl(l1));
	uiScrollSetChild(*s, uiControl(l2));
	uiScrollSetChild(*s, uiControl(l3));
	uiControlDestroy(uiControl(l2));
}

#define scrollUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int scrollRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		scrollUnitTest(scrollSetChildNoCrash),
		scrollUnitTest(scrollReplaceChildNoCrash),
	};

	return cmocka_run_group_tests_name("uiScroll", tests, unitTestsSetup, unitTestsTeardown);
}
