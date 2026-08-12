#include "unit.h"

// uiImageBuffer is tied to a uiDrawContext, so the test drives a real uiArea
// and exercises the buffer API from inside the draw handler.
//
// There is no way to read pixels back through the public API, so the test
// verifies the full create -> update -> draw -> free cycle runs without
// failing; a failing buffer operation crashes or aborts the test via
// uiprivImplBug() on the backends.

#define IMWIDTH 64
#define IMHEIGHT 64

static int drawCalls;
static int drawFailed;

static void fillPixels(unsigned char *pixels, int width, int height)
{
	int x, y;
	size_t i;

	for (y = 0; y < height; y++)
		for (x = 0; x < width; x++) {
			i = ((size_t) y * width + x) * 4;
			pixels[i] = (unsigned char) (x * 255 / width);		// red
			pixels[i + 1] = (unsigned char) (y * 255 / height);	// green
			pixels[i + 2] = (unsigned char) (255 - x * 255 / width);	// blue
			pixels[i + 3] = 255;					// alpha
		}
}

static void drawBuffer(uiDrawContext *c, int alpha)
{
	uiImageBuffer *buf;
	unsigned char *pixels;
	uiRect src;
	uiRect dst;

	pixels = (unsigned char *) malloc((size_t) IMWIDTH * IMHEIGHT * 4);
	if (pixels == NULL) {
		drawFailed = 1;
		return;
	}
	fillPixels(pixels, IMWIDTH, IMHEIGHT);

	buf = uiNewImageBuffer(c, IMWIDTH, IMHEIGHT, alpha);
	uiImageBufferUpdate(buf, pixels);
	free(pixels);

	// full-size render
	src.X = 0;
	src.Y = 0;
	src.Width = IMWIDTH;
	src.Height = IMHEIGHT;
	dst.X = 0;
	dst.Y = 0;
	dst.Width = IMWIDTH;
	dst.Height = IMHEIGHT;
	uiImageBufferDraw(c, buf, &src, &dst, 0);

	// scaled down, with bilinear filtering
	src.Width = IMWIDTH / 2;
	src.Height = IMHEIGHT / 2;
	dst.X = IMWIDTH + 5;
	dst.Y = 0;
	dst.Width = IMWIDTH / 2;
	dst.Height = IMHEIGHT / 2;
	uiImageBufferDraw(c, buf, &src, &dst, 1);

	// scaled up, nearest neighbor
	src.Width = IMWIDTH / 4;
	src.Height = IMHEIGHT / 4;
	dst.X = IMWIDTH + IMWIDTH / 2 + 15;
	dst.Y = 0;
	dst.Width = IMWIDTH;
	dst.Height = IMHEIGHT;
	uiImageBufferDraw(c, buf, &src, &dst, 0);

	uiFreeImageBuffer(buf);
}

static void areaDraw(uiAreaHandler *a, uiArea *area, uiAreaDrawParams *p)
{
	drawBuffer(p->Context, 1);
	drawBuffer(p->Context, 0);
	drawCalls++;
}

static void areaMouseEvent(uiAreaHandler *a, uiArea *area, uiAreaMouseEvent *e) {}
static void areaMouseCrossed(uiAreaHandler *ah, uiArea *a, int left) {}
static void areaDragBroken(uiAreaHandler *ah, uiArea *a) {}
static int areaKeyEvent(uiAreaHandler *ah, uiArea *a, uiAreaKeyEvent *e) { return 0; }

static void imageBufferDraw(void **state)
{
	struct state *s = *state;
	uiAreaHandler handler;
	uiArea *area;
	int i;

	drawCalls = 0;
	drawFailed = 0;

	handler.Draw = areaDraw;
	handler.MouseEvent = areaMouseEvent;
	handler.MouseCrossed = areaMouseCrossed;
	handler.DragBroken = areaDragBroken;
	handler.KeyEvent = areaKeyEvent;
	area = uiNewArea(&handler);

	uiWindowSetChild(s->w, uiControl(area));
	uiControlShow(uiControl(area));
	uiControlShow(uiControl(s->w));

	// pump the message loop until the area has painted; the first step
	// blocks so the window is displayed on macOS, where a non-blocking
	// step that finds no queued event returns without running the display
	// cycle
	uiMainSteps();
	uiMainStep(1);
	for (i = 0; i < 99 && drawCalls == 0; i++)
		uiMainStep(0);

	assert_true(drawCalls > 0);
	assert_false(drawFailed);
}

#define imageBufferUnitTest(f) cmocka_unit_test_setup_teardown((f), \
		unitTestSetup, unitTestTeardown)

int imageBufferRunUnitTests(void)
{
	const struct CMUnitTest tests[] = {
		imageBufferUnitTest(imageBufferDraw),
	};

	return cmocka_run_group_tests_name("uiImageBuffer", tests, unitTestsSetup, unitTestsTeardown);
}
