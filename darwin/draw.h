// 6 january 2017

// TODO why do we still have this file; should we just split draw.m or not

struct uiDrawContext {
	CGContextRef c;
	CGFloat height;				// needed for text; see below
};

// bitmap API
struct uiDrawBitmap {
	CGImageRef cg;
	unsigned char *data;
	int Width;
	int Height;
	int Stride;
};
