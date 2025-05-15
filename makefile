CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g

SRCS = main.cpp emulator_core.cpp emulator_enhanced.cpp memory_manager.cpp os_core.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = os_executable

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

test: $(TARGET)
	./$(TARGET) test

clean:
	rm -f $(OBJS) $(TARGET)