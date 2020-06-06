#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main(void){
	int fd;
	char buf;
	fd = open("/dev/stopwatch", O_RDWR);
	if (fd < 0){
		printf("Open Failured!\n");
		return -1;
	}
	write(fd, &buf, 1);
	close(fd);
	return 0;
}
