CFLAGS=-Wall -std=c++17 -O2 -fopenmp
LFLAGS=-O2
LIBS=-fopenmp

.PHONY: all clean
all: accessDram

accessDram : main.o config.o addressTranslation.o
	g++ ${LFLAGS} -o accessDram main.o config.o addressTranslation.o ${LIBS}

main.o: main.cpp config.h addressTranslation.h
	g++ -c ${CFLAGS} -o main.o main.cpp

config.o: config.cpp config.h
	g++ -c ${CFLAGS} -o config.o config.cpp

addressTranslation.o: addressTranslation.cpp addressTranslation.h
	g++ -c ${CFLAGS} -o addressTranslation.o addressTranslation.cpp

clean:
	rm -f accessDram *.o

