CC = x86_64-w64-mingw32-gcc -Werror
CFLAGS = -std=c99 -O2 -IC:/Progra~1/Assimp/include
LDFLAGS = -mconsole -g
LDLIBS = -lkernel32 -LC:/Progra~1/Assimp/lib/x64 -lassimp-vc140-mt

model-compressor.exe: main.c
	$(CC) $(LDFLAGS) $(CFLAGS) fastlz.c main.c -o $@ $(LDLIBS)

clean:
	rm -f *.exe *.o
