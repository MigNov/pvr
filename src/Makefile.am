CC=gcc
OUT=pvr
XML_LIBS=`pkg-config libxml-2.0 --libs`
XML_CFLAGS=`pkg-config libxml-2.0 --cflags`
LIBS=-lvirt $(XML_LIBS)
CFLAGS+=-Wall -g -DHAVE_CONFIG_H

EXTRA_DIST = xml.c devwatch.c mountutils.c vmrunner.c gtk-vncviewer.c gtk-selector.c utils.c vmrunner.h

if H_BLKID
LIBS += -lblkid
endif

if H_GTKVNC
GTK_LIBS=-lgtk-vnc-1.0 -lgtk-x11-2.0 -lglib-2.0 -lcairo -lpangocairo-1.0 -lpango-1.0 -lgdk-x11-2.0 -lgobject-2.0
GTK_INCLUDES= -I/usr/include/gtk-vnc-1.0 -I/usr/include/gtk-2.0 -I/usr/lib64/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib64/glib-2.0/include -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/gvnc-1.0
endif

all:
	$(CC) $(CFLAGS) -o $(OUT) xml.c devwatch.c mountutils.c vmrunner.c gtk-vncviewer.c gtk-selector.c utils.c $(LIBS) $(GTK_LIBS) $(GTK_INCLUDES) $(XML_CFLAGS)

install-exec-local:
	if [ ! -z "$(DESTDIR)" ]; then $(MKDIR) -p $(DESTDIR)/usr/bin; $(CP) pvr $(DESTDIR)/usr/bin; fi
