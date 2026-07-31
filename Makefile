CXX      ?= g++
CXXFLAGS ?= -std=c++20 -O2 -flto -Wall -Wextra
LDFLAGS  ?=
LIBS     ?= -lftxui-component -lftxui-dom -lftxui-screen -lvlc
OBJS     := main.o player.o util.o config.o ui.o splash.o

all: mozart

mozart: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

main.o: main.cpp player.h ui.h config.h util.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

player.o: player.cpp player.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

util.o: util.cpp util.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

config.o: config.cpp config.h util.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

ui.o: ui.cpp ui.h splash.h player.h util.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

splash.o: splash.cpp splash.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f mozart *.o

.PHONY: all clean
