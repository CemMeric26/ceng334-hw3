# Compiler
CXX = g++
CC = gcc

# Compiler flags
CXXFLAGS = -Wall -std=c++11
CFLAGS = -Wall

# Source files
SRCS = recext2fs.cpp identifier.cpp ext2fs_print.c

# Object files
OBJS = recext2fs.o identifier.o ext2fs_print.o

# Executable name
TARGET = recext2fs

# Build the executable
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compile C++ source files
recext2fs.o: recext2fs.cpp
	$(CXX) $(CXXFLAGS) -c recext2fs.cpp

identifier.o: identifier.cpp
	$(CXX) $(CXXFLAGS) -c identifier.cpp

# Compile C source files
ext2fs_print.o: ext2fs_print.c
	$(CC) $(CFLAGS) -c ext2fs_print.c

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
