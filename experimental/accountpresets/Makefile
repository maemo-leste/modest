# Makefile for testing ModestAccountPresets

test-modest-account-presets: modest-account-presets.o test-modest-account-presets.o
	$(CC) -o test-modest-account-presets test-modest-account-presets.o modest-account-presets.o `pkg-config --libs gobject-2.0`

modest-account-presets.o: modest-account-presets.c modest-account-presets.h
	$(CC) -c modest-account-presets.c -g -Wall `pkg-config --cflags gobject-2.0`

test-modest-account-presets.o: test-modest-account-presets.c modest-account-presets.h
	$(CC) -c test-modest-account-presets.c -g -Wall `pkg-config --cflags gobject-2.0`

clean:
	rm -f *.o *~ core
