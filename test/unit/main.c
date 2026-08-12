#include <stdio.h>
#include <string.h>

#include "unit.h"

int unitWindowOnClosingQuit(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

int unitTestsSetup(void **state)
{
	*state = malloc(sizeof(struct state));
	assert_non_null(*state);
	return 0;
}

int unitTestsTeardown(void **state)
{
	free(*state);
	return 0;
}

int unitTestSetup(void **_state)
{
	struct state *state = *_state;
	uiInitOptions o = {0};

	assert_null(uiInit(&o));
	state->c = NULL;
	state->w = uiNewWindow(UNIT_TEST_WINDOW_TITLE, UNIT_TEST_WINDOW_WIDTH, UNIT_TEST_WINDOW_HEIGHT, 0);
	uiWindowOnClosing(state->w, unitWindowOnClosingQuit, NULL);
	return 0;
}

int unitTestTeardown(void **_state)
{
	struct state *state = *_state;

	if (state->c != NULL)
		uiWindowSetChild(state->w, uiControl(state->c));
	uiControlShow(uiControl(state->w));
	//uiMain();
	uiMainSteps();
	uiMainStep(1);
	uiControlDestroy(uiControl(state->w));
	uiUninit();
	return 0;
}

struct unitTest {
	const char *name;
	int (*fn)(void);
};

int main(int argc, char *argv[])
{
	size_t i;
	int failedTests = 0;
	int failedComponents = 0;
	const char *filter = NULL;
	struct unitTest unitTests[] = {
		{ "init", initRunUnitTests },
		{ "window", windowRunUnitTests },
		{ "menu", menuRunUnitTests },
		{ "slider", sliderRunUnitTests },
		{ "spinbox", spinboxRunUnitTests },
		{ "label", labelRunUnitTests },
		{ "button", buttonRunUnitTests },
		{ "combobox", comboboxRunUnitTests },
		{ "editablecombobox", editableComboboxRunUnitTests },
		{ "checkbox", checkboxRunUnitTests },
		{ "radiobuttons", radioButtonsRunUnitTests },
		{ "tab", tabRunUnitTests },
		{ "scroll", scrollRunUnitTests },
		{ "entry", entryRunUnitTests },
		{ "progressbar", progressBarRunUnitTests },
		{ "drawmatrix", drawMatrixRunUnitTests },
		{ "imagebuffer", imageBufferRunUnitTests },
		{ "attrstr", attrstrRunUnitTests },
		{ "tooltip", tooltipRunUnitTests },
	};

	// an optional argument filters which component(s) run; each component
	// runs as its own process so ctest reports them individually
	if (argc > 1)
		filter = argv[1];

	for (i = 0; i < sizeof(unitTests)/sizeof(*unitTests); ++i) {
		int fails;
		if (filter != NULL && strstr(unitTests[i].name, filter) == NULL)
			continue;
		fails = (unitTests[i].fn)();
		failedTests += fails;
		if (fails > 0)
			failedComponents++;
	}

	puts("[==========]");
	if (failedTests == 0)
		puts("[  PASSED  ] All test(s) in all component(s).");
	else
		printf("[  FAILED  ] %d test(s) in %d component(s), see above.\n",
			       failedTests, failedComponents);

	return failedTests;
}
