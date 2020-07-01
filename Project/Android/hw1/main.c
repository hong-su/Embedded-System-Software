#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/sem.h> 
#include <sys/shm.h>
#include <sys/mman.h>
#include <linux/input.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <dirent.h>
#include <termios.h>
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

#define BUFF_SIZE 64
#define KEY_RELEASE 0
#define KEY_PRESS 1
#define NOPS 1
#define SLEEP 200000 
#define SWITCH_MAX_BUTTON 9

#define MAX_DIGIT 4
#define FND_DEVICE "/dev/fpga_fnd"

#define LCD_MAX_BUFF 32
#define LCD_LINE_BUFF 16
#define LCD_DEVICE "/dev/fpga_text_lcd"

#define FPGA_BASE_ADDRESS 0x08000000 //fpga_base address
#define LED_ADDR 0x16 

#define DOT_DEVICE "/dev/fpga_dot"

#define MODE_CONTINUE 99

union semun { 
    int val; 
    struct semid_ds *buf; 
    unsigned short *array; 
}; 

struct sembuf p1 = {0, -1, SEM_UNDO }, p2 = {1, -1, SEM_UNDO }, p3 = {2, -1, SEM_UNDO };
struct sembuf v1 = {0, 1, SEM_UNDO }, v2 = {1, 1, SEM_UNDO }, v3 = {2, 1, SEM_UNDO };

int getsem (void){
	union semun x;
	x.val = 0;
	int id = -1;

    id = (int)semget (IPC_PRIVATE, 3, 0600 | IPC_CREAT);
    semctl ( id, 0, SETVAL, x);
    semctl ( id, 1, SETVAL, x);
    semctl ( id, 2, SETVAL, x);

	return id;
}

int main(void){
    struct my_pid{
        pid_t input;
        pid_t output;
    }pid;

    int *shmaddr; 
    int shmid; 
    shmid = shmget(IPC_PRIVATE, 200, IPC_CREAT|0644); 
    if(shmid == -1) { 
        perror("shmget"); 
        return -1;
    } 

    int semid = getsem();	 
    pid.input = fork();
    if(pid.input == 0){
        /********************************************************** 
         * input process 
         * *******************************************************/
        struct input_event ev[BUFF_SIZE];
        int fd, rd, size = sizeof (struct input_event);

        if((fd = open ("/dev/input/event0", O_RDONLY|O_NONBLOCK)) == -1) {
            printf ("%s is not a vaild device.n", "/dev/input/event0");
        }
        
        int fd_s;
        int buff_size_s;
        unsigned char push_sw_buff[SWITCH_MAX_BUTTON];
        if((fd_s = open ("/dev/fpga_push_switch", O_RDWR)) < 0) { 
            printf ("%s is not a vaild device.n", "/dev/fpga_push_switch");
        }
        buff_size_s = sizeof(push_sw_buff);
        
        //printf("sleep p2\n");
        semop (semid, &p2, NOPS);
        //printf("wake p2\n");

        shmaddr = (int *) shmat(shmid, (char *) NULL, 0); 
        while (1){
            usleep(SLEEP);

            rd = read (fd, ev, (size_t)size * BUFF_SIZE);
            read (fd_s, &push_sw_buff, (size_t)buff_size_s);
            
            int i, j;
            unsigned short int siwtch_flag = 0;
            for(i = 0, j = 1; i < SWITCH_MAX_BUTTON; i++, j*=2) {
                siwtch_flag |= push_sw_buff[i]*j;
            }
            
            if(siwtch_flag != 0){
                shmaddr[0] = siwtch_flag;
                //printf("wake p1\n");
                semop (semid, &v1, NOPS); 
                semop (semid, &p2, NOPS); 
            }

            if(rd > 0 && ev[0].value == 1){
                shmaddr[0] = ev[0].code;
                //printf("wake p1\n");
                semop (semid, &v1, NOPS);  

                //if(ev[0].code == 158) {
                if(ev[0].code == 114) { 
                    //shmaddr[1] = 0;
                    exit(0);
                }
                semop (semid, &p2, NOPS); 
            }
            if(shmaddr[1] == MODE_CONTINUE){
                semop (semid, &v1, NOPS);  
                semop (semid, &p2, NOPS); 
            }
        }
    }
    else{
        pid.output = fork();
        if(pid.output == 0){
            /********************************************************** 
             * output process 
             * *******************************************************/
            int mode;
            unsigned char dot_set_blank[10] = {
                0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
            };

            /********************************************************** 
             * mode_1 variable
             * *******************************************************/
            int fnd_dev;
            unsigned char fnd_data[4];

            int lcd_dev;
            unsigned char lcd_string[32];

            int led_dev;
            unsigned long *fpga_addr = 0;
            unsigned char *led_addr = 0;

            /********************************************************** 
             * mode_2 variable
             * *******************************************************/
            int led[4] = {64, 32, 16, 128};

            /********************************************************** 
             * mode_3 variable
             * *******************************************************/
            //int count;
            int dot_dev;
            size_t dot_str_size;

            unsigned char fpga_dot[2][10] = {
                {0x3E,0x7F,0x63,0x63,0x63,0x7F,0x7F,0x63,0x63,0x63}, // A
                {0x0c,0x1c,0x1c,0x0c,0x0c,0x0c,0x0c,0x0c,0x0c,0x1e} // 1
            };
            int dot_flag_arr[4] = {0, 0, 0, 1};
            int press_num;
            bool dot_flag;

            /********************************************************** 
             * mode_4 variable
             * *******************************************************/
            unsigned char dot_edit[10];
            unsigned char cursor_point_x_arr[7] = {64, 32, 16, 8, 4, 2, 1};
            struct cp{
                int x;
                int y;
            }cursor_point;

            shmaddr = (int *) shmat(shmid, (char *) NULL, 0); 
          
            lcd_dev = open(LCD_DEVICE, O_WRONLY);
            if (lcd_dev < 0) {
                printf("Device open error : %s\n", LCD_DEVICE);
                return -1;
            }

            led_dev = open("/dev/mem", O_RDWR | O_SYNC);
            if (led_dev < 0) {
                perror("/dev/mem open error");
                return -1;
            }

            fpga_addr = (unsigned long *)mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, led_dev, FPGA_BASE_ADDRESS);
            if (fpga_addr == MAP_FAILED){
                printf("mmap error!\n");
                close(led_dev);
                return -1;
            }
            led_addr = (unsigned char*)((void*)fpga_addr+LED_ADDR);

            dot_dev = open(DOT_DEVICE, O_WRONLY);
            if (dot_dev < 0) {
                printf("Device open error : %s\n",DOT_DEVICE);
                exit(1);
            }
	
            while(1){
                fnd_dev = open(FND_DEVICE, O_RDWR);
                if (fnd_dev < 0) {
                    printf("Device open error : %s\n", FND_DEVICE);
                    return -1;
                }
                //printf("sleep p3\n");
                semop (semid, &p3, NOPS); 
                //printf("wake p3\n");
                
another_mode:
               // if(shmaddr[0] == 158){ //Shutdown
                if(shmaddr[0] == 114){ //Shutdown
                    memset(lcd_string, ' ', sizeof(lcd_string));	
                    write(lcd_dev, lcd_string, LCD_MAX_BUFF);
                    memset(fnd_data, 0, sizeof(fnd_data));	
                    write(fnd_dev, fnd_data, 4);
                    write(dot_dev, dot_set_blank, sizeof(dot_set_blank));	

                    *led_addr = 0;
                    close(lcd_dev);
                    close(led_dev);
                    close(dot_dev);
                    close(fnd_dev);
                    munmap(led_addr, 4096);

                    semop (semid, &v1, NOPS); 
                    exit(0);
                }

                mode = shmaddr[0];
            
                //Set dot blank
                write(dot_dev, dot_set_blank, sizeof(dot_set_blank));	
                
                if(mode == 0){
                    memset(lcd_string, ' ', sizeof(lcd_string));	
                    write(lcd_dev, lcd_string, LCD_MAX_BUFF);
                    if(shmaddr[1] == MODE_CONTINUE){                       
                        int k = 0;
                        while(1){
                            if(k <= 5){
                                *led_addr = 32;
                            }
                            else{
                                *led_addr = 16;
                            }
                            if(k%10 == 0) k = 0;

                            memcpy(fnd_data, &(shmaddr[2]), sizeof(fnd_data));
                            write(fnd_dev, fnd_data, 4);
                  
                            semop (semid, &v2, NOPS); 
                            semop (semid, &p3, NOPS); 

                            //Mode Change
                            if(shmaddr[0] != mode){ 
                                shmaddr[1] = 0;
                                goto another_mode;
                            } // or Edit exit
                            else if(shmaddr[1] == 0) break;
                            k++;  
                        }
                    }
                    *led_addr = 128;
                    memcpy(fnd_data, &(shmaddr[2]), sizeof(fnd_data));
                    write(fnd_dev, fnd_data, 4);
     
                    close(fnd_dev);
                }
                else if(mode == 1){
                    memset(lcd_string, ' ', sizeof(lcd_string));	
                    write(lcd_dev, lcd_string, LCD_MAX_BUFF);
                   *led_addr = (unsigned char)led[shmaddr[1]];
                    memcpy(fnd_data, &(shmaddr[2]), sizeof(fnd_data));
                    write(fnd_dev, fnd_data, 4);
                }
                else if(mode == 2){
                    *led_addr = 0;
                    memcpy(fnd_data, &(shmaddr[2]), sizeof(fnd_data));
                    memcpy(lcd_string, &(shmaddr[5]), sizeof(lcd_string));
                    memcpy(fnd_data, &(shmaddr[2]), sizeof(fnd_data));
                    write(fnd_dev, fnd_data, 4);
                    write(lcd_dev, lcd_string, LCD_MAX_BUFF);
                    press_num = shmaddr[4];
                    dot_flag = dot_flag_arr[press_num];
                    dot_str_size = sizeof(fpga_dot[dot_flag]);
                    write(dot_dev, fpga_dot[dot_flag], dot_str_size);	  
                }
                else if(mode == 3){
                    *led_addr = 0;
                    if(shmaddr[1] == MODE_CONTINUE){
                        bool dot_chk;
                        int k = 0;
                        while(1){
                            memcpy(fnd_data, &(shmaddr[2]), sizeof(fnd_data));
                            write(fnd_dev, fnd_data, 4);

                            cursor_point.x = shmaddr[3];
                            cursor_point.y = shmaddr[4];
                            dot_chk = shmaddr[5];
                            memcpy(dot_edit, &(shmaddr[6]), sizeof(dot_edit));
                            if(dot_chk == 1){
                                if(k > 5){
                                    dot_edit[cursor_point.y] -= cursor_point_x_arr[cursor_point.x];
                                }   
                            }
                            else{
                                if(k <= 5){
                                    dot_edit[cursor_point.y] += cursor_point_x_arr[cursor_point.x];
                                }
                            }
                            if(k%10 == 0) k = 0;
                            
                            write(dot_dev, dot_edit, sizeof(dot_edit));	  
                            semop (semid, &v2, NOPS); 
                            semop (semid, &p3, NOPS); 
                        
                            //Mode Change
                            if(shmaddr[0] != mode){
                                shmaddr[1] = 0;
                                goto another_mode;
                            } // or Edit exit
                            else if(shmaddr[1] == 0) break;
                            k++;
                        }
                    }
                    
                    memcpy(dot_edit, &(shmaddr[6]), sizeof(dot_edit));
                    memcpy(fnd_data, &(shmaddr[2]), sizeof(fnd_data));
                    write(fnd_dev, fnd_data, 4);
                    write(dot_dev, dot_edit, sizeof(dot_edit));	  
                }
                //printf("wake p2-sig\n");
                close(fnd_dev);
                semop (semid, &v2, NOPS); 
            }
            munmap(led_addr, 4096);
            //close(fnd_dev);
            close(lcd_dev);
            close(led_dev);
            close(dot_dev);
        }
        else{
            /********************************************************** 
             * main process 
             * *******************************************************/
            
            int mode = 0;
            int input_button = 0;
            int i;
            bool keep = 0;
            unsigned char fnd_data[4];

            shmaddr = (int *) shmat(shmid, (char *) NULL, 0); 
            shmaddr[0] = 0; // init

            //Nonvolatile variable
                //Mode 1 First init
            time_t now = time(NULL);
            time_t time_tmp;

                //mode_4
            unsigned char dot_edit[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            bool dot_chk[7][10];

            while(1){
                input_button = shmaddr[0];
                //if(input_button == 158) {
                if(input_button == 114) { // vol down  -Shutdown
                    semop (semid, &v3, NOPS); 
                    semop (semid, &p1, NOPS);
                    return 0;
                }

                else if(input_button == 115){ //vol up
                    mode++;
                    mode %= 4;
                    keep = 0;
                    input_button = 0;
                }
                shmaddr[0] = mode;
                if(mode == 0){
                    /********************************************************** 
                     * mode_1 variable
                     * *******************************************************/
                    struct tm *now_tm;
                    int hour, min;
                    
                    if(keep == 0){
                        time_tmp = now;
                        shmaddr[1] = 0;
                        keep++;
                    }
                    if(input_button == 1){ //edit
                        if(shmaddr[1] == 0) { //start
                            shmaddr[1] = MODE_CONTINUE;
                        }
                        else { //end
                            now = time_tmp;
                            shmaddr[1] = 0;
                        }
                    }
                    else if(input_button == 2 && shmaddr[1] == MODE_CONTINUE){ //reset
                        time_tmp = now;
                        shmaddr[1] = 0;
                    }
                    else if(input_button == 4 && shmaddr[1] == MODE_CONTINUE){ //imp min
                        time_tmp += 60*60;
                    }
                    else if(input_button == 8 && shmaddr[1] == MODE_CONTINUE){ //imp hour
                        time_tmp += 60;
                    }
                    now_tm = localtime(&time_tmp);
                    hour = now_tm->tm_hour;
                    min = now_tm->tm_min;

                    fnd_data[0] = (unsigned char)hour/10;
                    fnd_data[1] = (unsigned char)hour%10;
                    fnd_data[2] = (unsigned char)min/10;
                    fnd_data[3] = (unsigned char)min%10;
                    memcpy(&(shmaddr[2]), fnd_data, sizeof(fnd_data));
                }
                else if(mode == 1){
                    /********************************************************** 
                     * mode_2 variable
                     * *******************************************************/
                    int num, num_tmp, sys_tmp;
                    int sys[4] = {10, 8, 4, 2};

                    if(keep == 0){
                        num = 0;
                        shmaddr[1] = 0;
                        keep++;
                    }
                    if(input_button == 1){
                        shmaddr[1]++;
                        shmaddr[1]%=4;  
                    }
                    else if(input_button == 2){
                        num += sys[shmaddr[1]]*sys[shmaddr[1]];
                    }
                    else if(input_button == 4){
                        num += sys[shmaddr[1]];
                    }
                    else if(input_button == 8){
                        num++;
                    }

                    num_tmp = num;
                    sys_tmp = num_tmp%sys[shmaddr[1]];
                    fnd_data[3] = (unsigned char)sys_tmp;
                    num_tmp /= sys[shmaddr[1]];

                    sys_tmp = num_tmp%sys[shmaddr[1]];
                    fnd_data[2] = (unsigned char)sys_tmp;
                    num_tmp /= sys[shmaddr[1]];

                    sys_tmp = num_tmp%sys[shmaddr[1]];
                    fnd_data[1] = (unsigned char)sys_tmp;
                    num_tmp /= sys[shmaddr[1]];

                    if(sys[shmaddr[1]] == 10) fnd_data[0] = 0;
                    else{
                        sys_tmp = num_tmp%sys[shmaddr[1]];
                        fnd_data[0] = (unsigned char)sys_tmp;
                        num_tmp /= sys[shmaddr[1]];
                    }
                    memcpy(&(shmaddr[2]), fnd_data, sizeof(fnd_data));
                }
                else if(mode == 2){
                    /********************************************************** 
                     * mode_3 variable
                     * *******************************************************/
                    int count;
                    int press_num;
                    int input_button_prev;
                    int str_size;
                    unsigned char lcd_string[LCD_MAX_BUFF + 1];
                    char text[4][9] = {
                        {'.', 'A', 'D', 'G', 'J', 'M', 'P', 'T', 'W'},
                        {'Q', 'B', 'E', 'H', 'K', 'N', 'R', 'U', 'X'},
                        {'Z', 'C', 'F', 'I', 'L', 'O', 'S', 'V', 'Y'},
                        {'1', '2', '3', '4', '5', '6', '7', '8' ,'9'}
                    };
                    int find_input_num[9] = {1, 2, 4, 8, 16, 32, 64, 128, 256};
                    int input_num, num_tmp;

                    for(input_num = 0; input_num < 9; input_num++){
                        if(input_button == find_input_num[input_num]) break;
                    }

                    if(keep == 0){
                        press_num = 0;
                        count = -1;
                        str_size = 0;
                        memset(lcd_string, ' ', sizeof(lcd_string) - sizeof(char));	
                        lcd_string[LCD_MAX_BUFF] = '\0';
                        input_button = input_button_prev = 0;
                        keep++;
                        goto space;
                    }

                    else if(input_button == 2+4){
                        press_num = 0;
                        count = -1;
                        str_size = 0;
                        memset(lcd_string, ' ', sizeof(lcd_string) - sizeof(char));	
                        input_button = input_button_prev = 0;
                        goto space;
                    }
                    else if(input_button == 16+32){ //Change alpabet <-> number
                        if(press_num == 3) press_num = 0;
                        else press_num = 3;
                    }
                    else if(input_button == 128+256){ //Space
                        if(str_size == LCD_MAX_BUFF - 1){
                            for(i=0; i < LCD_MAX_BUFF - 1; i++)
                                lcd_string[i] = lcd_string[i+1];
                            str_size--;
                        }
                        if(str_size != 0) str_size++;
                        lcd_string[str_size] = ' ';
                        goto space;
                    }
                    else if(count == 0){
                        lcd_string[str_size] = text[press_num][input_num];
                    }
                    else if(press_num == 3){
                        if(str_size == LCD_MAX_BUFF - 1){
                            for(i=0; i < LCD_MAX_BUFF - 1; i++)
                                lcd_string[i] = lcd_string[i+1];
                            str_size--;
                        }
                        if(count == 1){
                            lcd_string[str_size] = text[press_num][input_num];
                        }
                        else{
                            lcd_string[++str_size] = text[press_num][input_num];
                        }
                    }
                    else if(input_button != input_button_prev) {
                        if(press_num != 3) press_num = 0;
                        if(str_size == LCD_MAX_BUFF - 1){
                            for(i=0; i < LCD_MAX_BUFF - 1; i++)
                                lcd_string[i] = lcd_string[i+1]; 
                            str_size--;
                        }
                        lcd_string[++str_size] = text[press_num][input_num];
                    }
                    else{
                        lcd_string[str_size] = text[press_num][input_num];
                    }
                    if(press_num != 3){
                        press_num++;
                        press_num%=3;
                    }
                    space:      
                    input_button_prev = input_button;

                    count++;
                    num_tmp = count;
                    fnd_data[3] = (unsigned char)num_tmp%10;
                    num_tmp /= 10;

                    fnd_data[2] = (unsigned char)num_tmp%10;
                    num_tmp /= 10;

                    fnd_data[1] = (unsigned char)num_tmp%10;
                    num_tmp /= 10;

                    fnd_data[0] = (unsigned char)num_tmp%10;
                    num_tmp /= 10;
                    memcpy(&(shmaddr[2]), fnd_data, sizeof(fnd_data));
                    shmaddr[4] = press_num;
                    memcpy(&(shmaddr[5]), lcd_string, sizeof(lcd_string));
                }
                else if(mode == 3){
                    /********************************************************** 
                     * mode_4 variable
                     * *******************************************************/
                    struct cp{
                        int x;
                        int y;
                    }cursor_point;
                    unsigned char cursor_point_x_arr[7] = {64, 32, 16, 8, 4, 2, 1};
                    int j;
                    int count;
                    int num_tmp;

                    if(keep == 0){
                        cursor_point.x = 0;
                        cursor_point.y = 0;
                        for(i=0; i<10; i++){
                            dot_edit[i] = 0;
                        }
                        for(i=0; i<7; i++){
                            for(j=0; j<10; j++){
                                dot_chk[i][j] = 0;
                            }
                        }
                        count = -1;
                        shmaddr[1] = MODE_CONTINUE;
                        keep++;
                    }
                    if(input_button == 1){ //reset
                        cursor_point.x = 0;
                        cursor_point.y = 0;
                        for(i=0; i<10; i++){
                            dot_edit[i] = 0;
                        }
                        for(i=0; i<7; i++){
                            for(j=0; j<10; j++){
                                dot_chk[i][j] = 0;
                            }
                        }
                        count = -1;
                        shmaddr[1] = MODE_CONTINUE;
                    }
                    else if(input_button == 2){ //up
                        if(cursor_point.y > 0) cursor_point.y--;
                        else  cursor_point.y = 9;
                    }
                    else if(input_button == 8){ //left
                        if(cursor_point.x > 0) cursor_point.x--;
                        else cursor_point.x = 6;
                    }
                    else if(input_button == 32){ //right
                        if(cursor_point.x < 6) cursor_point.x++;
                        else cursor_point.x = 0;
                    }
                    else if(input_button == 128){ //down
                        if(cursor_point.y < 9) cursor_point.y++;
                        else cursor_point.y = 0;
                    }
                    else if(input_button == 4){ //cursor
                        if(shmaddr[1] == MODE_CONTINUE) shmaddr[1] = 0;
                        else shmaddr[1] = MODE_CONTINUE;
                    }
                    
                    else if(input_button == 16){ //select 
                        if(dot_chk[cursor_point.x][cursor_point.y]){
                            dot_chk[cursor_point.x][cursor_point.y] = 0;
                            dot_edit[cursor_point.y] -= cursor_point_x_arr[cursor_point.x];
                        }
                        else{
                            dot_chk[cursor_point.x][cursor_point.y] = 1;
                            dot_edit[cursor_point.y] += cursor_point_x_arr[cursor_point.x];
                        }
                    }
                    
                    else if(input_button == 64){ //clear
                        for(i=0; i<10; i++){
                            dot_edit[i] = 0;
                        }
                    }
                    
                    else if(input_button == 256){ //mirror
                        for(i = 0; i < 7; i++){
                            for(j = 0; j < 10; j++){
                                if(dot_chk[i][j] == 1){
                                    dot_chk[i][j] = 0;
                                    dot_edit[j] -= cursor_point_x_arr[i];
                                }
                                else{
                                    dot_chk[i][j] = 1;
                                    dot_edit[j] += cursor_point_x_arr[i];
                                }
                            }
                        }
                    }

                    if(input_button != 3) count++;

                    num_tmp = count;
                    fnd_data[3] = (unsigned char)num_tmp%10;
                    num_tmp /= 10;

                    fnd_data[2] = (unsigned char)num_tmp%10;
                    num_tmp /= 10;

                    fnd_data[1] = (unsigned char)num_tmp%10;
                    num_tmp /= 10;

                    fnd_data[0] = (unsigned char)num_tmp%10;
                    num_tmp /= 10;
                    memcpy(&(shmaddr[2]), fnd_data, sizeof(fnd_data));

                    shmaddr[3] = cursor_point.x;
                    shmaddr[4] = cursor_point.y;
                    shmaddr[5] = dot_chk[cursor_point.x][cursor_point.y];
                    memcpy(&(shmaddr[6]), dot_edit, sizeof(dot_edit));
                }
                //printf("wake-sig p3\n");
                semop (semid, &v3, NOPS); 
                //printf("sleep p1\n");
                semop (semid, &p1, NOPS); 
            }
        }
    }
}