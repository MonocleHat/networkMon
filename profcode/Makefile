CC=g++
CFLAGS=-I
CFLAGS+=-Wall
#CFLAGS+=-DEBUG
FILES2=netmon.cpp
FILES1=intfmon.cpp

intfMonitor: .
	$(CC) $(CFLAGS) $(FILES1) -o intfMonitor

networkMonitor: .
	$(CC) $(CFLAGS) $(FILES2) -o networkMonitor

clean:
	rm -f *.o intfMonitor networkMonitor

all: intfMonitor networkMonitor
