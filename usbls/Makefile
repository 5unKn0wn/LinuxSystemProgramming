CC = gcc
CFLAGS = -c -o
LDFLAGS = -lusb-1.0 -ludev
OBJECTS = usbls.o names.o
INCLUDES = -I/usr/include/libusb-1.0/
TARGET = usbls
SOURCE = usbls.c names.c

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) usbls.o usbls.c $(INCLUDES) $(LDFLAGS)
	$(CC) $(CFLAGS) names.o names.c $(INCLUDES) $(LDFLAGS)
	$(CC) -o $@ $(OBJECTS) $(INCLUDES) $(LDFLAGS)

clean:
	rm *.o $(TARGET)
