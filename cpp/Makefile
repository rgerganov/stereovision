CC = g++
PROGNAME = stereo
CFLAGS = -Wall -g
LDFLAGS = -lSDL -ljpeg

all: $(PROGNAME)

$(PROGNAME): $(PROGNAME).cpp camera.h camera.cpp jpeg_util.cpp
	$(CC) $(PROGNAME).cpp camera.cpp jpeg_util.cpp -o $(PROGNAME) $(CFLAGS) $(LDFLAGS)

tags:
	ctags *.cpp *.h

clean:
	rm -f $(PROGNAME) *.o

