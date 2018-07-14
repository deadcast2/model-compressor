CC = x86_64-w64-mingw32-gcc -Werror
CFLAGS = -std=c99 -O2
LDFLAGS = -mconsole -g
LDLIBS = -lkernel32

model-compressor.exe: main.c
	$(CC) $(LDFLAGS) $(CFLAGS) fastlz.c main.c -o $@ $(LDLIBS)

clean:
	rm -f *.exe *.o
