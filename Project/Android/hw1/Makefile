CC = arm-none-linux-gnueabi-gcc
TARGET = app
OBJS = main.c

$(TARGET):$(OBJS)
	$(CC) -Wall -Wextra -static -o $(TARGET) $(OBJS)

clean:
	rm  $(TARGET)
