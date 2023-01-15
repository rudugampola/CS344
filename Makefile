# the compiler: gcc for C program, define as g++ for C++
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -Wextra -Wpedantic -Werror

# the build target executable:
TARGET = base64enc

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) -std=c99 $(CFLAGS) -o $(TARGET) $(TARGET).c

clean:
	$(RM) $(TARGET)
