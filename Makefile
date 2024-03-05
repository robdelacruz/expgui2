TSRC=t.c db.c clib.c mainwin.c
T2SRC=t2.c db.c clib.c tui.c tuilib.c
LIBSRC=

# pkg-config --cflags --libs gtk+-3.0
GTK_CFLAGS=-pthread -I/usr/include/gtk-3.0 -I/usr/include/at-spi2-atk/2.0 -I/usr/include/at-spi-2.0 -I/usr/include/dbus-1.0 -I/usr/lib/x86_64-linux-gnu/dbus-1.0/include -I/usr/include/gtk-3.0 -I/usr/include/gio-unix-2.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/fribidi -I/usr/include/harfbuzz -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/uuid -I/usr/include/freetype2 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng16 -I/usr/include/x86_64-linux-gnu -I/usr/include/libmount -I/usr/include/blkid -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include

GTK_LIBS=-lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -lharfbuzz -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0

CFLAGS= -std=gnu99 -Wall -Werror
CFLAGS+= -Wno-unused 
CFLAGS+= -Wno-deprecated-declarations 
CFLAGS+= -g
LIBS=

all: t t2

dep:
	apt install libgtk-3-dev

#.SILENT:
t: $(TSRC) sqlite3.o
	gcc $(CFLAGS) $(GTK_CFLAGS) -o $@ $^ $(LIBS) $(GTK_LIBS)

sqlite3.o: sqlite3/sqlite3.c
	gcc -c -o $@ $<

termbox2.o: termbox2.c
	gcc -c -o $@ $^

t2: $(T2SRC) sqlite3.o termbox2.o
	gcc $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf t t2 *.o

