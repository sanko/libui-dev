// 3 january 2025
#include "uipriv_unix.h"

struct uiScroll {
	uiUnixControl c;
	GtkWidget *widget;
	GtkWidget *viewport;
	uiprivChild *child;
};

uiUnixControlAllDefaultsExceptDestroy(uiScroll)

static void uiScrollDestroy(uiControl *c)
{
	uiScroll *s = uiScroll(c);

	if (s->child != NULL)
		uiprivChildDestroy(s->child);
	g_object_unref(s->widget);
	uiFreeControl(uiControl(s));
}

void uiScrollSetChild(uiScroll *s, uiControl *child)
{
	if (s->child != NULL)
		uiprivChildRemove(s->child);
	s->child = uiprivNewChild(child, uiControl(s), GTK_CONTAINER(s->viewport));
}

uiScroll *uiNewScroll(void)
{
	uiScroll *s;

	uiUnixNewControl(uiScroll, s);

	s->widget = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(s->widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(s->widget), GTK_SHADOW_NONE);

	// we create the viewport ourselves (instead of letting GtkScrolledWindow wrap the child) so that we can
	// (re)place the child inside it without having to worry about the one-child restriction of the scrolled window
	s->viewport = gtk_viewport_new(
		gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(s->widget)),
		gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(s->widget)));
	gtk_widget_show(s->viewport);
	gtk_container_add(GTK_CONTAINER(s->widget), s->viewport);

	return s;
}
