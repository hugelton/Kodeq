CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra
SRCS = parser.cpp simulator.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = reelia_simulator

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)