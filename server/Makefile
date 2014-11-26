CFLAGS	= 
LDFLAGS	= -ljson -lmysqlcppconn -lcrypto

#-lmysqlcppconn == Database connector lib
#-lcrypto == SHA1 lib

all:
#	gcc $(CFLAGS) -o main $(LDFLAGS) main.c
	g++ -std=c++0x -pthread -o ${BUILD}/server main.cpp $(CFLAGS) $(LDFLAGS)

clean:
	rm ${BUILD}/*