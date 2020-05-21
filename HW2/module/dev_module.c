#include <linux/module.h>
#include <linux/fs.h>
#include <asm/io.h>
#include <asm/uaccess.h>

#include <linux/platform_device.h>
#include <linux/kernel.h>

#include "./fpga_dot_font.h"

#define DEVICE_MAJOR 242					/* Device Driver Major Number */
#define DEVICE_NAME "dev_driver"			/* Device Driver Name */

#define FND_ADDRESS 0x08000004 				/* FND Physical Address */
#define LED_ADDRESS 0x08000016		 		/* LED Physical Address */
#define DOT_ADDRESS 0x08000210 				/* DOT Physical Address */
#define TEXT_LCD_ADDRESS 0x08000090 		/* Text LCD Physical Address - 32 Byte (16 * 2) */

#define SUCCESS 0
#define STUDENT_NUMBER "20151556"
#define STUDENT_NAME "Byun HongSu"
#define STUDENT_NUMBER_LENTH 8
#define STUDENT_NAME_LENTH 11
#define TEXT_LCD_MAX_LENTH 32

#define IOCTL_SET_OPTION _IOW(DEVICE_MAJOR, 0, char *)
#define IOCTL_COMMAND _IOW(DEVICE_MAJOR, 1, int *)

#define CUSTOM_HZ HZ/10

/* Global Variable */
static int Device_Open = 0;
static unsigned char *fnd_addr;
static unsigned char *led_addr;
static unsigned char *dot_addr;
static unsigned char *text_lcd_addr;

int led_number[8] = {128, 64, 32, 16, 8, 4, 2, 1};
int fnd_index;
int number_index;
int number_index_rest;
int number_count;

struct line_shift{
	int line1;
	int line2;
} lcd_shift;
int line1_sign;
int line2_sign;

struct timer_list timer;
int timer_interval;
int timer_count;
int counter;

/* Function Prototype */
int __init device_init(void);
void __exit device_exit(void);
int device_open(struct inode *inode, struct file *file);
int device_release(struct inode *inode, struct file *file);
void device_write_fnd(int index, char number);
void device_write_led(int index);
void device_write_dot(int index);
void device_write_text_lcd(struct line_shift lcd_shift);
void calc_lcd_shift(void);
void kernel_timer_write(unsigned long timeout);
long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param);

/* 
* Define file_operations structure.
* This structure will hold the functions to be called when a process does something to the device we created. 
* Since a pointer to this structure is kept in the devices table, it can't be local to init_module.
* NULL is for unimplemented functions.
*/
struct file_operations fops = {
	.owner				=	THIS_MODULE,
	.open 				=	device_open,
	.unlocked_ioctl 	= 	device_ioctl,	
	.release			=	device_release
};

/* 
* Module Declarations 
*/

/* Initialize the module − Register the character device */
int __init device_init(void) {
	int result;
	result = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &fops);
	if(result < 0) {
		printk(KERN_WARNING "Can't get any major\n");
		return result;
	}
	led_addr = ioremap(LED_ADDRESS, 0x1);
	fnd_addr = ioremap(FND_ADDRESS, 0x4);
	dot_addr = ioremap(DOT_ADDRESS, 0x10);
	text_lcd_addr = ioremap(TEXT_LCD_ADDRESS, 0x32);

	init_timer(&(timer));
	printk("init module, %s major number : %d\n", DEVICE_NAME, DEVICE_MAJOR);

	return 0;
}

/* Exit the moudle - UnRegister the character device */
void __exit device_exit(void) {
	unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
	iounmap(fnd_addr);
	iounmap(led_addr);
	iounmap(dot_addr);
	iounmap(text_lcd_addr);
	del_timer_sync(&timer);
	printk("Bye");
}

/* 
* Function Declarations 
*/

int device_open(struct inode *inode, struct file *file) {
	/* Don't want to run to two processes at the same time */
	if(Device_Open) 
		return -EBUSY;
	Device_Open++;
	return SUCCESS;
}

int device_release(struct inode *inode, struct file *file) {
	/* Now Ready for our next caller */
	Device_Open--;
	return SUCCESS;
}

/* Wrtie FND Device */
void device_write_fnd(int index, char number) {
	unsigned short int value_short = 0;
	unsigned char value[4];
	value[0] = value[1] = value[2] = value[3] = 0;
	value[index] = number;
	value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];
    outw(value_short,(unsigned int)fnd_addr);	    
}

/* Write LED Device */
void device_write_led(int index) {
	unsigned short _s_value;
    _s_value = (unsigned short)led_number[index-1];
    outw(_s_value, (unsigned int)led_addr);
}

/* Write Dot Device */
void device_write_dot(int index) {
	int i;
	unsigned short int _s_value;
	unsigned char value[10];
	
	for(i=9; i>=0; i--){
		value[i] = fpga_number[index][i];
	}

	for(i=9; i>=0; i--){
        _s_value = value[i] & 0x7F;
		outw(_s_value,(unsigned int)dot_addr+i*2);
    }
}

/* Write TEXT LCD Device */
void device_write_text_lcd(struct line_shift lcd_shift) {
	int i;
    unsigned short int _s_value = 0;
	unsigned char value[TEXT_LCD_MAX_LENTH + 1];
	int HALF_LENTH = TEXT_LCD_MAX_LENTH/2;

	value[TEXT_LCD_MAX_LENTH] = 0;
	for(i = TEXT_LCD_MAX_LENTH - 1; i >= 0; i--){
		value[i] = ' ';
	}

	for(i = STUDENT_NUMBER_LENTH - 1; i >= 0; i--){
		value[i+lcd_shift.line1] = STUDENT_NUMBER[i];
	}

	for(i = STUDENT_NAME_LENTH - 1; i >= 0; i--){
		value[i + HALF_LENTH + lcd_shift.line2] = STUDENT_NAME[i];
	}
	/*
	for(i=0; i<STUDENT_NAME_LENTH; i++){
		value[i+HALF_LENTH+lcd_shift.line2] = STUDENT_NAME[i];
	}
	*/
	for(i=0; i < TEXT_LCD_MAX_LENTH; i+=2){
        _s_value = (value[i] & 0xFF) << 8 | (value[i + 1] & 0xFF);
		outw(_s_value,(unsigned int)text_lcd_addr+i);
    }
}

/* Calculate TEXT LCD line Shift vlaue */
void calc_lcd_shift(){
	lcd_shift.line1 += line1_sign;
	lcd_shift.line2 += line2_sign;
	if(lcd_shift.line1%8 == 0){
		line1_sign *= -1;
	}
	if(lcd_shift.line2%5 == 0){
		line2_sign *= -1;
	}
}

/* Write Timer */
void kernel_timer_write(unsigned long timeout) {
	int number_count = counter%8;
	counter++;
	if(counter == timer_count){
		/* Print 0 All of Using Devic */
		int i;
		outw(0,(unsigned int)fnd_addr);	    
		outw(0, (unsigned int)led_addr);
		for(i=0; i<10; i++){
			outw(0,(unsigned int)dot_addr+i*2);
  		}
		for(i=0; i<TEXT_LCD_MAX_LENTH; i+=2) {
			outw(0x2020, (unsigned int)text_lcd_addr+i);
    	}
		return ;
	}
	if(counter%8 == 0){
		fnd_index++;
		fnd_index %= 4;
	}
	number_index = (number_count + number_index_rest)%8 + 1;
	calc_lcd_shift();
	device_write_fnd(fnd_index, (char)number_index);
	device_write_led(number_index);
	device_write_dot(number_index);
	device_write_text_lcd(lcd_shift);

	timer.expires = get_jiffies_64() + (timer_interval * CUSTOM_HZ); 
	timer.function = kernel_timer_write;
	/* Not use 
	* timer.data =
	*/
	add_timer(&timer);
}

long device_ioctl(struct file *file, unsigned int ioctl_num, unsigned long ioctl_param){
	char gdata[4];
	int i;
	int data_param[2];
	switch (ioctl_num) {
	
	/* Initialize Device */
	case IOCTL_SET_OPTION:
	
		/* Ignore exceed 4bytes */
		if (copy_from_user(&gdata, (char *)ioctl_param, 4))
			return -EFAULT;
		for(i=3; i>=0; i--){
			if(gdata[i] != '0') break;
		}

		fnd_index = i;
		number_index_rest = (int)gdata[i] - 0x30;
		number_index = number_index_rest;
		
		/* Error : Start Number > 8 */
		if(number_index > 8) 
			return EINVAL;

		lcd_shift.line1 = 0;
		lcd_shift.line2 = 0;
		line1_sign = 1;
		line2_sign = 1;

		device_write_fnd(fnd_index, (char)number_index);
		device_write_led(number_index);
		device_write_dot(number_index);
		device_write_text_lcd(lcd_shift);
		break;
	
	/* Run Command */
	case IOCTL_COMMAND:
		if (copy_from_user(&data_param, (int *)ioctl_param, 8))
			return -EFAULT;

		counter = 0;
		timer_interval = data_param[0];
		timer_count = data_param[1];
		del_timer_sync(&timer);
		timer.expires = jiffies + (timer_interval * CUSTOM_HZ); 
		timer.function = kernel_timer_write;
		/* Not use 
		* timer.data =
		*/ 
		add_timer(&timer);
		break;

	default:
		break;
	}
	return SUCCESS;
}

module_init(device_init);
module_exit(device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huins");
