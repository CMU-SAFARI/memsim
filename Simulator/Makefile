all: bin/OoOTraceSimulator bin/Debug.OoOTraceSimulator bin/Prof.OoOTraceSimulator
debug1: bin/Debug.OoOTraceSimulator

CPPFLAGS = -O3 -lm 
DEBUGFLAGS = -lm -g 
PROFFLAGS = -lm -pg 
SRCS = ComponentList.cc
HEADERS = $(wildcard *.h)

bin/OoOTraceSimulator: OoOTraceSimulator.cc $(SRCS) $(HEADERS) Makefile
	g++ $(CPPFLAGS) $< $(SRCS) -lz -o $@ 

bin/Debug.OoOTraceSimulator: OoOTraceSimulator.cc $(SRCS) $(HEADERS) Makefile
	g++ $(DEBUGFLAGS) $< -lz $(SRCS) -lz -o $@ 

bin/Prof.OoOTraceSimulator: OoOTraceSimulator.cc $(SRCS) $(HEADERS) Makefile
	g++ $(PROFFLAGS) $< $(SRCS) -lz -o $@ 

clean:
	rm -f bin/Debug.OoOTraceSimulator bin/OoOTraceSimulator bin/Prof.OoOTraceSimulator
