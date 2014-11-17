CFLAGS	= $(shell pkg-config --cflags json-c)
LDFLAGS	= $(shell pkg-config --libs json-c)

all:
#	gcc $(CFLAGS) -o main $(LDFLAGS) main.c
	g++ -pthread -o server main.cpp $(CFLAGS) $(LDFLAGS)

clean:
	rm main