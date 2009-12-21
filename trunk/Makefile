LIBPURPLE_CFLAGS = -I/usr/include/libpurple -I/usr/local/include/libpurple -DPURPLE_PLUGINS -DENABLE_NLS
GLIB_CFLAGS = -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include -I/usr/local/include
GAMMU_CFLAGS = -I/usr/include/gammu -lGammu

all:
	gcc libgammu.c -o libgammu.so -shared -dynamic ${LIBPURPLE_CFLAGS} ${GLIB_CFLAGS} ${GAMMU_CFLAGS} -g -Wall -pipe -fPIC -DPIC
