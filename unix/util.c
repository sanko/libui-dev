// 18 april 2015
#include "uipriv_unix.h"

void uiSetAppMetadata(const char *name, const char *version, const char *package)
{
	(void) version;
	// the application name is what GTK shows in its About dialog and elsewhere
	if (name != NULL)
		g_set_application_name(name);
	// the prgname becomes the WM_CLASS of the windows and the basis of the
	// GApplication ID, so prefer the package identifier
	if (package != NULL)
		g_set_prgname(package);
}

void uiprivSetMargined(GtkContainer *c, int margined)
{
	if (margined)
		gtk_container_set_border_width(c, uiprivGTKXMargin);
	else
		gtk_container_set_border_width(c, 0);
}
