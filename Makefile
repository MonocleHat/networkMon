netmon: netmon.cpp
	g++ -Wall netmon.cpp -o netmon -lpthread
intfmon: intfmon.cpp
	g++ -Wall intfmon.cpp -o intfmon

all: netmon intfmon
