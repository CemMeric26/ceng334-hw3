CXX = g++
CC = gcc
CXXFLAGS = -Wall -g
CFLAGS = -Wall -g
LDFLAGS = -lm

# Source files
CPP_SRCS = recext2fs.cpp identifier.cpp
C_SRCS = ext2fs_print.c

# Object files
CPP_OBJS = $(CPP_SRCS:.cpp=.o)
C_OBJS = $(C_SRCS:.c=.o)
OBJS = $(CPP_OBJS) $(C_OBJS)

# Executable name
EXEC = recext2fs

# Default target
all: $(EXEC)

# Link the executable
$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

# Compile C++ source files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Compile C source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and executable
clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean
