CFLAGS	= 
LDFLAGS	= -ljson -lmysqlcppconn -lcrypto -lssl -ldl

#-lmysqlcppconn == Database connector lib
#-lcrypto == SHA1 lib

all:
#	gcc $(CFLAGS) -o main $(LDFLAGS) main.c

	g++ -std=c++0x -c -Wall -Werror -fpic -o ${BUILD}/Dropbox.o lib/Dropbox.cpp
	g++ -std=c++0x -shared -o /usr/local/lib/libdropbox.so ${BUILD}/Dropbox.o
	g++ -std=c++0x -c -Wall -Werror -fpic -o ${BUILD}/GoogleDrive.o lib/GoogleDrive.cpp
	g++ -std=c++0x -shared -o /usr/local/lib/libgoogledrive.so ${BUILD}/GoogleDrive.o

	g++ -std=c++0x -pthread -o ${BUILD}/server main.cpp $(CFLAGS) $(LDFLAGS)

clean:
	[ -f ${BUILD}/* ] && rm -r ${BUILD}/*
	[ -f /usr/local/lib/libdropbox.so ] && sudo rm /usr/local/lib/libdropbox.so
	[ -f /usr/local/lib/libgoogledrive.so ] && sudo rm /usr/local/lib/libgoogledrive.so

default:
	echo nothing done!
	exit
