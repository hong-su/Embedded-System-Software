#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#define DEVICE_MAJOR 242							
#define DEVICE_NAME "dev_driver"	
#define IOCTL_SET_OPTION _IOW(DEVICE_MAJOR, 0, char *)
#define IOCTL_COMMAND _IOW(DEVICE_MAJOR, 1, int *)

int main(int argc, char **argv){
	int i;
	int fd;
	int argv3 = 0;
	int command[2];

	/* Passed Argument number is not 3	*/
	if(argc != 4) { 
		printf("Usage : [TIMER_INTERVAL] [TIMER_CNT] [TIMER_ INIT]\n");
		return -1;
	}

	/* Timer interval is not valid */
	for(i=strlen(argv[1])-1; i>=0; i--){
		if(isdigit(argv[1][i]) == 0){
			printf("Usage : [TIMER_INTERVAL 1-100] [TIMER_CNT] [TIMER_ INIT]\n");
			return -1;
		}
	}

	/* Timer count is not valid */
	for(i=strlen(argv[2])-1; i>=0; i--){
		if(isdigit(argv[2][i]) == 0){
			printf("Usage : [TIMER_INTERVAL] [TIMER_CNT 1-100] [TIMER_ INIT]\n");
			return -1;
		}
	}

	/* Timer init is not valid */
	if(strlen(argv[3]) != 4){
		printf("Usage : [TIMER_INTERVAL] [TIMER_CNT] [TIMER_ INIT 0001-8000]\n");
		return -1;
	}
	for(i=strlen(argv[3])-1; i>=0; i--){
		if(isdigit(argv[3][i]) == 0 || argv[3][i] > '8'){
			printf("Usage : [TIMER_INTERVAL] [TIMER_CNT] [TIMER_ INIT 0001-8000]\n");
			return -1;
		}
		if(argv[3][i] != '0') argv3++;
	}
	if(argv3 != 1){
		printf("Usage : [TIMER_INTERVAL] [TIMER_CNT] [TIMER_ INIT 0001-8000]\n");
		return -1;
	}

	/* 
	* Passed Argument is valid 
	*/
	fd = open("/dev/dev_driver", O_WRONLY);
	if (fd<0){
		printf("Open Failured!\n");
		return -1;
	}
	ioctl(fd, IOCTL_SET_OPTION, argv[3]);
	command[0] = atoi(argv[1]);
	command[1] = atoi(argv[2]);
	ioctl(fd, IOCTL_COMMAND, command);
	close(fd);
	
	return 0;
}
