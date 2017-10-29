CXXFLAGS=-std=c++1z -Wall -Wextra -Og -g -I. -include defl.h
CXX=g++

CFLAGS=-std=gnu11 -Wall -Wextra -Og -g
CC=gcc

SOURCES=painbox.cpp helper.cpp socket.cpp $(addprefix sysimp/,read.cpp write.cpp accept.cpp connect.cpp)
DEPS=$(SOURCES:.cpp=.d)
OBJECTS=$(SOURCES:.cpp=.o)

EXAMPLES=client server quorum_server rdlog_sender rdlog_receiver simplog_sender
EXAMPLES_SRC=$(addsuffix .c,$(EXAMPLES))

all: painbox $(EXAMPLES)

painbox: $(OBJECTS)
	$(CXX) -o painbox $(OBJECTS) -lstdc++

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $< -MD

%: %.c
	$(CC) $(CFLAGS) -o $@ $< -MD -static

-include $(DEPS) $(EXAMPLES_SRC:.c=.d)

clean:
	-rm -f $(OBJECTS) $(DEPS) painbox client server $(EXAMPLES) *.m4 *.pdf *.inc *.dot $(EXAMPLES_SRC:.c=.d)
