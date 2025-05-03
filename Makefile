CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
SRCS = parser.cpp expression.cpp simulator.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = kodeq_simulator

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $

clean:
	rm -f $(OBJS) $(TARGET)