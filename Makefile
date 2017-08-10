all: soundd libsoundd soundc

soundd: soundd.o
	$(CC) $(CFLAGS) $(LDFLAGS) soundd.c -o soundd -lasound

libsoundd: soundclient.o
	$(CC) $(CFLAGS) $(LDFLAGS) -fpic -shared soundclient.c -o libsoundd.so 

soundc: soundc.o libsoundd
	$(CC) $(CFLAGS) $(LDFLAGS) soundc.c -o soundc -lsoundd -L.


clean: 
	rm *.o *.so *.c~ *.h~


install:
	install soundd $(INSTALL_DIR_PREFIX)/usr/bin	
	install soundc $(INSTALL_DIR_PREFIX)/usr/bin
	install libsoundd.so $(INSTALL_DIR_PREFIX)/usr/lib
	install soundclient.h $(INSTALL_DIR_PREFIX)/usr/include

