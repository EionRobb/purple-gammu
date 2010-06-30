LIBPURPLE_CFLAGS = -I/usr/include/libpurple -I/usr/local/include/libpurple -DPURPLE_PLUGINS -DENABLE_NLS -lglib-2.0
GLIB_CFLAGS = -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include -I/usr/local/include
GAMMU_CFLAGS = -I/usr/include/gammu -lGammu

all:	libgammu.c
	gcc libgammu.c -o libgammu.so -shared -dynamic ${LIBPURPLE_CFLAGS} ${GLIB_CFLAGS} ${GAMMU_CFLAGS} -g -Wall -pipe -fPIC -DPIC


libgammu.dll:	libgammu.c
	i586-mingw32-gcc libgammu.c -o libgammu.dll -shared -dynamic ${LIBPURPLE_CFLAGS} ${GLIB_CFLAGS} ${GAMMU_CFLAGS} -g -Wall -pipe -mno-cygwin -L./Gammu-1.27.93-Windows-Minimal/lib -L/root/pidgin/pidgin-2.3.0_win32/libpurple -L/root/pidgin/win32-dev/gtk_2_0/lib -lws2_32 -lsetupapi -lpurple
	upx libgammu.dll
