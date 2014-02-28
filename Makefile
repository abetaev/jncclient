all: ncsvc-wrapper.so

CFLAGS = -fPIC -std=c99 -m32
LDFLAGS = -shared -m32
LIBS=-L/usr/lib32 -ldl -lc -lm -lnetlink

ncsvc-wrapper.so: ncsvc-wrapper.o ll_map.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) *.o *.so
