CC=gcc
CFLAGS=-Wall

all: clean static_build shared_build static shared

static_build:
	$(CC) $(CFLAGS) -c finder.c -o finder.o
	ar rcsv libfinder.a finder.o

shared_build:
	$(CC) $(CFLAGS) -c -fPIC finder.c
	$(CC) $(CFLAGS) -shared -Wl,-soname,libfinder.so.1 -o libfinder.so.1.0.1 finder.o -lc
	ln -s libfinder.so.1.0.1 libfinder.so.1
	ln -s libfinder.so.1 libfinder.so

clean:
	rm -f main_static main_shared tmp
	rm -rf *.o *.so *.a *.so.*

static: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o
	$(CC) $(CFLAGS) main.o libfinder.a -o main_static

shared: main.c
	$(CC) $(CFLAGS) main.c -lfinder -L. -o main_shared -Wl,-rpath,.


# ar rcsv - archiver, który archiwizuje wiele plików .o w jeden .a
# -L<path> - gdzie gcc powinien szukać plików biblioteki
# -l<library_name> - które biblioteki powinny być załadowane
# -shared, -fPIC - opcje do biblioteki współdzielonej