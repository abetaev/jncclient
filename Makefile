all: ncsvc-wrapper.so

CFLAGS = -fPIC -std=c99 -m32
LDFLAGS = -shared -m32
LIBS=-L/usr/lib32 -ldl -lc -lm

ncsvc-wrapper.so: ncsvc-wrapper.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

ncsvc-wrapper: ncsvc-wrapper.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBS)

clean:
	$(RM) ncsvc-wrapper.o ncsvc-wrapper.so
