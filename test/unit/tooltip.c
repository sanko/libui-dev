#include "unit.h"

#define uiButtonPtrFromState(s) uiControlPtrFromState(uiButton, s)
#define uiEditableComboboxPtrFromState(s) uiControlPtrFromState(uiEditableCombobox, s)
#define uiRadioButtonsPtrFromState(s) uiControlPtrFromState(uiRadioButtons, s)

static void tooltipSetNull(void **state)
{
	uiButton **b = uiButtonPtrFromState(state);

	*b = uiNewButton("Text");
	uiControlSetTooltip(uiControl(*b), NULL);
}

static void tooltipSetValue(void **state)
{
	uiButton **b = uiButtonPtrFromState(state);

	*b = uiNewButton("Text");
	uiControlSetTooltip(uiControl(*b), "Tooltip text");
	uiControlSetTooltip(uiControl(*b), NULL);
}

static void tooltipEditableCombobox(void **state)
{
	uiEditableCombobox **c = uiEditableComboboxPtrFromState(state);

	*c = uiNewEditableCombobox();
	uiEditableComboboxAppend(*c, "Item");
	uiControlSetTooltip(uiControl(*c), "Tooltip text");
	uiControlSetTooltip(uiControl(*c), NULL);
}

static void tooltipRadioButtons(void **state)
{
	uiRadioButtons **r = uiRadioButtonsPtrFromState(state);

	*r = uiNewRadioButtons();
	uiRadioButtonsAppend(*r, "One");
	uiRadioButtonsAppend(*r, "Two");
	uiControlSetTooltip(uiControl(*r), "Tooltip text");
	uiControlSetTooltip(uiControl(*r), NULL);
}

#define tooltipUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int tooltipRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		tooltipUnitTest(tooltipSetNull),
		tooltipUnitTest(tooltipSetValue),
		tooltipUnitTest(tooltipEditableCombobox),
		tooltipUnitTest(tooltipRadioButtons),
	};

	return cmocka_run_group_tests_name("tooltip", tests, unitTestsSetup, unitTestsTeardown);
}
