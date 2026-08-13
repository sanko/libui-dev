#include "unit.h"

#define uiLabelPtrFromState(s) uiControlPtrFromState(uiLabel, s)

static void dragDestinationNew(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	uiDragDestination *dd;

	*c = uiNewLabel("drag destination");
	dd = uiNewDragDestination();
	assert_non_null(dd);
	uiControlRegisterDragDestination(uiControl(*c), dd);
}

static void dragDestinationAcceptTypesDefault(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	uiDragDestination *dd;

	*c = uiNewLabel("drag destination");
	dd = uiNewDragDestination();
	uiControlRegisterDragDestination(uiControl(*c), dd);
	assert_int_equal(uiDragDestinationAcceptTypes(dd), 0);
}

static void dragDestinationSetAcceptTypes(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	uiDragDestination *dd;

	*c = uiNewLabel("drag destination");
	dd = uiNewDragDestination();
	uiDragDestinationSetAcceptTypes(dd, uiDragTypeText | uiDragTypeURIs);
	uiControlRegisterDragDestination(uiControl(*c), dd);
	assert_int_equal(uiDragDestinationAcceptTypes(dd), uiDragTypeText | uiDragTypeURIs);
}

static void dragDestinationLastOperationDefault(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	uiDragDestination *dd;

	*c = uiNewLabel("drag destination");
	dd = uiNewDragDestination();
	uiControlRegisterDragDestination(uiControl(*c), dd);
	assert_int_equal(uiDragDestinationLastDragOperation(dd), uiDragOperationNone);
}

static void dragDestinationRegisterTwice(void **state)
{
	uiLabel **c = uiLabelPtrFromState(state);
	uiDragDestination *dd1;
	uiDragDestination *dd2;

	*c = uiNewLabel("drag destination");
	dd1 = uiNewDragDestination();
	dd2 = uiNewDragDestination();
	uiControlRegisterDragDestination(uiControl(*c), dd1);
	uiControlRegisterDragDestination(uiControl(*c), dd2);
}

#define dragDestinationUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int dragDestinationRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		dragDestinationUnitTest(dragDestinationNew),
		dragDestinationUnitTest(dragDestinationAcceptTypesDefault),
		dragDestinationUnitTest(dragDestinationSetAcceptTypes),
		dragDestinationUnitTest(dragDestinationLastOperationDefault),
		dragDestinationUnitTest(dragDestinationRegisterTwice),
	};

	return cmocka_run_group_tests_name("uiDragDestination", tests,
		unitTestsSetup, unitTestsTeardown);
}
