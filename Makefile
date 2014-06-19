CC     = gcc
FLAGS  = -Wall
SRC    = sms.c
OBJ    = $(SRC:.c=.o)
TARGET = sms

all:$(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(FLAGS) -o $@ $^

.PHONY:clean
clean:
	rm -f $(TARGET) $(OBJ)

