# Mind -Wl,-export-dynamic in case, a signal handler is defined in the main file!
export CFLAGS=`pkg-config --cflags libglade-2.0`
export LDFLAGS=`pkg-config --libs libglade-2.0` -export-dynamic

TARGET=showme

MEMBERS=${TARGET}

DEPS = $(patsubst %,%.d,$(MEMBERS))
OBJECTS = $(patsubst %,%.o,$(MEMBERS))

all: ${TARGET}


${TARGET}: ${OBJECTS}
	${CC} -o ${TARGET} ${OBJECTS} ${CFLAGS} ${LDFLAGS}

clean:
	rm ${OBJECTS} ${TARGET}

-include ${DEPS}



