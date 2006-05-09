#include <gtk/gtk.h>
#include <glade/glade.h>
/*
 * #include "mysignals.h"
 */

int main (int argc,
          char *argv[])
{
    GladeXML *xml;
    GtkWidget *widget;

    gtk_init(&argc,
             &argv);
    glade_init();

    if (argc<2)
        return 0;

    /* load the interface */
    xml=glade_xml_new(argv[1], argc > 2 ? argv[2] : NULL, NULL);

    /* connect signal handlers */
    glade_xml_signal_autoconnect(xml);

    gtk_main();

    return 0;
}

