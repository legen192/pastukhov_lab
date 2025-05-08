CC = g++
CFLAGS = -std=c++17 -Wall
BOOST_LIBS = -lboost_system -lboost_thread -lboost_chrono -lpthread
INCLUDE_DIRS = 

TARGET = lab4
SRC = lab4.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -o $(TARGET) $(SRC) $(BOOST_LIBS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
