#include <glib.h>

typedef struct {
	gchar *display_name;
	gchar *email_address;
} AsDbusRecipient;

GList * asdbus_resolve_recipients (const gchar *name);
