CC=gcc
CFLAGS=-c -Wall
LDFLAGS=
SOURCES=clmon.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=clmon

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf $(OBJECTS) $(EXECUTABLE)