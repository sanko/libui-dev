#include "unit.h"

#define uiEditableComboboxPtrFromState(s) uiControlPtrFromState(uiEditableCombobox, s)

static void editableComboboxNew(void **state)
{
	uiEditableCombobox **c = uiEditableComboboxPtrFromState(state);

	*c = uiNewEditableCombobox();
}

static void editableComboboxPlaceholderDefault(void **state)
{
	uiEditableCombobox **c = uiEditableComboboxPtrFromState(state);
	char *rv;

	*c = uiNewEditableCombobox();
	rv = uiEditableComboboxPlaceholder(*c);
	assert_string_equal(rv, "");
	uiFreeText(rv);
}

static void editableComboboxSetPlaceholder(void **state)
{
	uiEditableCombobox **c = uiEditableComboboxPtrFromState(state);
	const char *text1 = "Placeholder 1";
	const char *text2 = "Placeholder 2";
	char *rv;

	*c = uiNewEditableCombobox();
	uiEditableComboboxSetPlaceholder(*c, text1);
	rv = uiEditableComboboxPlaceholder(*c);
	assert_string_equal(rv, text1);
	uiFreeText(rv);
	uiEditableComboboxSetPlaceholder(*c, text2);
	rv = uiEditableComboboxPlaceholder(*c);
	assert_string_equal(rv, text2);
	uiFreeText(rv);
}

#define editableComboboxUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int editableComboboxRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		editableComboboxUnitTest(editableComboboxNew),
		editableComboboxUnitTest(editableComboboxPlaceholderDefault),
		editableComboboxUnitTest(editableComboboxSetPlaceholder),
	};

	return cmocka_run_group_tests_name("uiEditableCombobox", tests, unitTestsSetup, unitTestsTeardown);
}
