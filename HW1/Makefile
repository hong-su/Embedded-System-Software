CC = arm-none-linux-gnueabi-gcc
TARGET = 20151556.out
OBJS = main.c

$(TARGET):$(OBJS)
	$(CC) -Wall -Wextra -static -o $(TARGET) $(OBJS)

clean:
	rm  $(TARGET)
