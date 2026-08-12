#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ui.h>

// The pixel data is generated once and stored in this buffer; it is
// re-uploaded to the renderer on every draw, since uiImageBuffer requires a
// draw context.
#define IMAGEBUFFER_WIDTH 256
#define IMAGEBUFFER_HEIGHT 256

uiWindow *mainwin;
uiArea *area;
uiAreaHandler handler;
unsigned char *pixels;
int mouseX;
int mouseY;

static void generatePixels(void)
{
	int x, y;
	size_t i;

	pixels = (unsigned char *) malloc((size_t) IMAGEBUFFER_WIDTH * IMAGEBUFFER_HEIGHT * 4);
	if (pixels == NULL) {
		fprintf(stderr, "out of memory\n");
		exit(1);
	}

	for (y = 0; y < IMAGEBUFFER_HEIGHT; y++)
		for (x = 0; x < IMAGEBUFFER_WIDTH; x++) {
			i = ((size_t) y * IMAGEBUFFER_WIDTH + x) * 4;
			pixels[i] = (unsigned char) x;				// blue
			pixels[i + 1] = (unsigned char) y;			// green
			pixels[i + 2] = (unsigned char) (255 - x);		// red
			pixels[i + 3] = 255;					// alpha
		}

	// draw a grid so the magnifier is visible
	for (y = 0; y < IMAGEBUFFER_HEIGHT; y++)
		for (x = 0; x < IMAGEBUFFER_WIDTH; x++)
			if (x % 32 == 0 || y % 32 == 0) {
				i = ((size_t) y * IMAGEBUFFER_WIDTH + x) * 4;
				pixels[i] = 0;
				pixels[i + 1] = 0;
				pixels[i + 2] = 0;
				pixels[i + 3] = 255;
			}
}

static void handlerDraw(uiAreaHandler *a, uiArea *area, uiAreaDrawParams *p)
{
	uiImageBuffer *buf;
	uiRect src;
	uiRect dst;

	buf = uiNewImageBuffer(p->Context, IMAGEBUFFER_WIDTH, IMAGEBUFFER_HEIGHT, 1);
	uiImageBufferUpdate(buf, pixels);

	// full-size render
	src.X = 0;
	src.Y = 0;
	src.Width = IMAGEBUFFER_WIDTH;
	src.Height = IMAGEBUFFER_HEIGHT;
	dst.X = 5;
	dst.Y = 5;
	dst.Width = IMAGEBUFFER_WIDTH;
	dst.Height = IMAGEBUFFER_HEIGHT;
	uiImageBufferDraw(p->Context, buf, &src, &dst, 0);

	// magnified view of the region around the mouse, with bilinear filtering
	if (mouseX >= 0 && mouseY >= 0) {
		src.X = mouseX - 16;
		src.Y = mouseY - 16;
		src.Width = 32;
		src.Height = 32;
		if (src.X < 0)
			src.X = 0;
		if (src.Y < 0)
			src.Y = 0;
		if (src.X + src.Width > IMAGEBUFFER_WIDTH)
			src.X = IMAGEBUFFER_WIDTH - src.Width;
		if (src.Y + src.Height > IMAGEBUFFER_HEIGHT)
			src.Y = IMAGEBUFFER_HEIGHT - src.Height;
		dst.X = 300;
		dst.Y = 5;
		dst.Width = 128;
		dst.Height = 128;
		uiImageBufferDraw(p->Context, buf, &src, &dst, 1);
	}

	uiFreeImageBuffer(buf);
}

static void handlerMouseEvent(uiAreaHandler *a, uiArea *area, uiAreaMouseEvent *e)
{
	mouseX = (int) e->X - 5;
	mouseY = (int) e->Y - 5;
	if (mouseX < 0 || mouseX >= IMAGEBUFFER_WIDTH)
		mouseX = -1;
	if (mouseY < 0 || mouseY >= IMAGEBUFFER_HEIGHT)
		mouseY = -1;
	uiAreaQueueRedrawAll(area);
}

static void handlerMouseCrossed(uiAreaHandler *ah, uiArea *a, int left)
{
	// do nothing
}

static void handlerDragBroken(uiAreaHandler *ah, uiArea *a)
{
	// do nothing
}

static int handlerKeyEvent(uiAreaHandler *ah, uiArea *a, uiAreaKeyEvent *e)
{
	// reject all keys
	return 0;
}

static int onClosing(uiWindow *w, void *data)
{
	uiControlDestroy(uiControl(mainwin));
	uiQuit();
	return 0;
}

static int shouldQuit(void *data)
{
	uiControlDestroy(uiControl(mainwin));
	return 1;
}

int main(void)
{
	uiInitOptions o;
	const char *err;
	uiBox *box;
	uiLabel *label;

	handler.Draw = handlerDraw;
	handler.MouseEvent = handlerMouseEvent;
	handler.MouseCrossed = handlerMouseCrossed;
	handler.DragBroken = handlerDragBroken;
	handler.KeyEvent = handlerKeyEvent;

	memset(&o, 0, sizeof (uiInitOptions));
	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "error initializing ui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	uiOnShouldQuit(shouldQuit, NULL);

	generatePixels();

	mainwin = uiNewWindow("libui ImageBuffer Example", 640, 480, 1);
	uiWindowSetMargined(mainwin, 1);
	uiWindowOnClosing(mainwin, onClosing, NULL);

	box = uiNewVerticalBox();
	uiWindowSetChild(mainwin, uiControl(box));

	area = uiNewArea(&handler);
	uiBoxAppend(box, uiControl(area), 1);

	label = uiNewLabel("Move the mouse over the image buffer to zoom in with the magnifier.");
	uiBoxAppend(box, uiControl(label), 0);

	uiControlShow(uiControl(mainwin));
	uiMain();
	uiUninit();
	return 0;
}
