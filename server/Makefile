CFLAGS	= $(shell pkg-config --cflags json-c)
LDFLAGS	= $(shell pkg-config --libs json-c)

all:
#	gcc $(CFLAGS) -o main $(LDFLAGS) main.c
	g++ -std=c++0x -pthread -o ${BUILD}/server main.cpp $(CFLAGS) $(LDFLAGS)

clean:
	[ -f server ] && rm server