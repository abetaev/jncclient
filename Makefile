all: ncsvcwrapper.so

CFLAGS = -fPIC -std=c99 -m32
LDFLAGS = -shared -m32
LIBS=-L/usr/lib32 -ldl -lc -lm

ncsvcwrapper.so: ncsvcwrapper.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

ncsvcwrapper: ncsvcwrapper.c rtwrapper.c logwrapper.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $< $(LIBS)

clean:
	$(RM) *.o *.so
