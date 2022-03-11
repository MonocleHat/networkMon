netmon: netmon_proto.cpp
	g++ -Wall netmon_proto.cpp -o netmon
intfmon: intfMon.cpp
	g++ -Wall intfMon.cpp -o intfMon

all: netmon intfmon
