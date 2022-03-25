netmon: netmon.cpp
	g++ -Wall netmon.cpp -o netmon -lpthread
intfmon: intfMon_alt.cpp
	g++ -Wall intfMon_alt.cpp -o intfmon

all: netmon intfmon
