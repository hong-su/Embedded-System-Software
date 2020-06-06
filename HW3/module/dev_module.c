#include <linux/module.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <asm/io.h>
//#include <mach/gpio.h>
#include <linux/gpio.h>

#define DEVICE_MAJOR 242					/* Device Driver Major Number */
#define DEVICE_NAME "stopwatch"				/* Device Driver Name */
#define FND_ADDRESS 0x08000004 				/* FND Physical Address */
#define SUCCESS 0

/* Global Variable */
static int Device_Open = 0;
static unsigned char *fnd_addr;
struct timer_list timer;
struct timer_list end_timer;
bool timer_continue;
unsigned long long start_time;
int keep_time;

wait_queue_head_t wq_write;
DECLARE_WAIT_QUEUE_HEAD(wq_write);

/* Function Prototype */
int stopwatch_open(struct inode *, struct file *);
int stopwatch_release(struct inode *, struct file *);
int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void kernel_timer_write(unsigned long timeout);
void device_write_fnd(int timer_time);
void stopwatch_end(unsigned long timeout);

irqreturn_t inter_handler1(int irq, void* dev_id);
irqreturn_t inter_handler2(int irq, void* dev_id);
irqreturn_t inter_handler3(int irq, void* dev_id);
irqreturn_t inter_handler4(int irq, void* dev_id);

/* 
* Define file_operations structure.
* This structure will hold the functions to be called when a process does something to the device we created. 
* Since a pointer to this structure is kept in the devices table, it can't be local to init_module.
* NULL is for unimplemented functions.
*/
static struct file_operations fops =
{
	.open = stopwatch_open,
	.write = stopwatch_write,
	.release = stopwatch_release,
};

irqreturn_t inter_handler1(int irq, void* dev_id) {
	int tmp_hz;

	if(timer_continue) {
		printk("Access denied : Timer is running.\n");
		return IRQ_HANDLED;
	}
	timer_continue = true;

	del_timer(&timer);
	start_time = get_jiffies_64();

	tmp_hz = (int)keep_time % (int)HZ;
	tmp_hz = (int)HZ - tmp_hz;

	printk("Start timer.\n");
	printk("raw data: %d, expire time: %d, ", keep_time, tmp_hz);
	device_write_fnd(keep_time);
	
	timer.expires = get_jiffies_64() + tmp_hz; 
	timer.function = kernel_timer_write;
	add_timer(&timer);
	return IRQ_HANDLED;
}

irqreturn_t inter_handler2(int irq, void* dev_id) {
	if(timer_continue){
		printk("Pause timer.\n");
		timer_continue = false;
		keep_time += (get_jiffies_64() - start_time);
	}
	return IRQ_HANDLED;
}

irqreturn_t inter_handler3(int irq, void* dev_id) {
	if(timer_continue) {
		printk("Access denied : Timer is running.\n");
		return IRQ_HANDLED;
	}
	printk("Reset timer.\n");
	keep_time = 0;
	device_write_fnd(0);
	return IRQ_HANDLED;
}

irqreturn_t inter_handler4(int irq, void* dev_id) {
	if(timer_continue) {
		printk("Access denied : Timer is running.\n");
		return IRQ_HANDLED;
	}
	if(gpio_get_value(IMX_GPIO_NR(5, 14))){
		printk("Keep timer.\n");
		del_timer(&end_timer);
	}
	else{
		printk("Exiting timer. Press the button 3seconds.\n");
		end_timer.expires = get_jiffies_64() + 3*HZ;
		end_timer.function = stopwatch_end;
		add_timer(&end_timer);
	}
	return IRQ_HANDLED;
}

int stopwatch_open(struct inode *inode, struct file *file){
	int ret;
	int irq;
	printk(KERN_ALERT "Open Module\n");

	/* Don't want to run to two processes at the same time */
	if(Device_Open) 
		return -EBUSY;
	Device_Open++;

	keep_time = 0;
	device_write_fnd(0);

	gpio_direction_input(IMX_GPIO_NR(1,11));
	irq = gpio_to_irq(IMX_GPIO_NR(1,11));
	ret=request_irq(irq, inter_handler1, IRQF_TRIGGER_FALLING, "home", 0);

	gpio_direction_input(IMX_GPIO_NR(1,12));
	irq = gpio_to_irq(IMX_GPIO_NR(1,12));
	ret=request_irq(irq, inter_handler2, IRQF_TRIGGER_FALLING, "back", 0);

	gpio_direction_input(IMX_GPIO_NR(2,15));
	irq = gpio_to_irq(IMX_GPIO_NR(2,15));
	ret=request_irq(irq, inter_handler3, IRQF_TRIGGER_FALLING, "volup", 0);

	gpio_direction_input(IMX_GPIO_NR(5,14));
	irq = gpio_to_irq(IMX_GPIO_NR(5,14));
	ret=request_irq(irq, inter_handler4, IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING, "voldown", 0);

	return SUCCESS;
}

int stopwatch_release(struct inode *inode, struct file *file){
	/* Now Ready for our next caller */
	Device_Open--;

	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 11)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(1, 12)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(2, 15)), NULL);
	free_irq(gpio_to_irq(IMX_GPIO_NR(5, 14)), NULL);
	
	printk(KERN_ALERT "Release Module\n");
	return SUCCESS;
}

int stopwatch_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos){
	printk("sleep on\n");
	interruptible_sleep_on(&wq_write);
	printk("write\n");
	return SUCCESS;
}

/* Initialize the module − Register the character device */
int __init stopwatch_init(void) {
	int result;
	result = register_chrdev(DEVICE_MAJOR, DEVICE_NAME, &fops);
	if(result < 0) {
		printk(KERN_WARNING "Can't get any major\n");
		return result;
	}
	fnd_addr = ioremap(FND_ADDRESS, 0x4);
	init_timer(&timer);
	init_timer(&end_timer);
	printk(KERN_ALERT "Init Module Success\n");
	printk("%s major number : %d\n", DEVICE_NAME, DEVICE_MAJOR);

	return SUCCESS;
}

/* Exit the moudle - UnRegister the character device */
void __exit stopwatch_exit(void) {
	unregister_chrdev(DEVICE_MAJOR, DEVICE_NAME);
	iounmap(fnd_addr);
	del_timer_sync(&timer);
	del_timer_sync(&end_timer);
	printk(KERN_ALERT "Remove Module Success\n");
}

/* Write Timer */
void kernel_timer_write(unsigned long timeout) {
	int timer_time;
	if(!timer_continue) return;
	timer_time = (get_jiffies_64() - start_time + keep_time);
	printk("raw data: %llu, expire time: %d, ", get_jiffies_64() - start_time + keep_time, HZ);
	device_write_fnd(timer_time);
	timer.expires = get_jiffies_64() + HZ; 
	timer.function = kernel_timer_write;
	add_timer(&timer);
}

void device_write_fnd(int timer_time){
	int min, sec;
	unsigned char value[4];
	unsigned short int value_short = 0;
	timer_time /= HZ;
	sec = timer_time%60;
	timer_time /= 60;
	min = timer_time%60;
	printk("timer: %02d:%02d\n", min, sec);
	value[0] = min/10;
	value[1] = min%10;
	value[2] = sec/10;
	value[3] = sec%10;
	value_short = value[0] << 12 | value[1] << 8 | value[2] << 4 | value[3];
    outw(value_short, (unsigned int)fnd_addr);	
}

void stopwatch_end(unsigned long timeout){
	device_write_fnd(0);
	printk("Wake Up\n");
	__wake_up(&wq_write, 1, 1, NULL);
}

module_init(stopwatch_init);
module_exit(stopwatch_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Huins");
