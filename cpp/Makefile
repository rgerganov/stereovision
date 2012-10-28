PROGNAME = hello
CFLAGS = -Wall
LDFLAGS = -lSDL -lSDL_image

all: $(PROGNAME)

$(PROGNAME): $(PROGNAME).cpp
	$(CC) $(PROGNAME).cpp -o $(PROGNAME) $(CFLAGS) $(LDFLAGS)

clean:
	rm -f $(PROGNAME) *.o

