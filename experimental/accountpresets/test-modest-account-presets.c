/* unit test for the ModestAccountPresets implementation */

#include "modest-account-presets.h"

int
main (int argc, char* argv[])
{
	GObject *obj;

	g_type_init ();

	obj = modest_account_presets_new ();
/* do something interesting with our brand new object */

	return 0;
}
