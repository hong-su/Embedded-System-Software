#include <jni.h>
#include <fcntl.h>
#include "android/log.h"
#include "./fpga_dot_font.h"

#define LOG_TAG "MyTag"
#define LOGV(...)   __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define MAX_BUTTON 9

jint JNICALL Java_com_embe2020_project_MainActivity_controlswitch(JNIEnv *env, jobject this){
	int i;
	int dev_switch;
	int buff_size;
	int count;
	int acc;
	unsigned char push_sw_buff[MAX_BUTTON];
	
	dev_switch = open("/dev/fpga_push_switch", O_RDWR);
	buff_size=sizeof(push_sw_buff);

	read(dev_switch, &push_sw_buff, buff_size);
	count = 0;
	for(i=0;i<MAX_BUTTON;i++) {
		if(push_sw_buff[i] == 1) count++;
	}

	close(dev_switch);
	return count;
};

void JNICALL Java_com_embe2020_project_MainActivity_deviceclear(JNIEnv *env, jobject this){
	int dev_dot;
	int dev_fnd;
	int dev_textlcd;
	int str_size;

	unsigned char string[32];
	unsigned char data[4];

	dev_dot = open("/dev/fpga_dot", O_WRONLY);
	dev_fnd = open("/dev/fpga_fnd", O_WRONLY);
	dev_textlcd = open("/dev/fpga_text_lcd", O_WRONLY);

	data[0] = data[1] = data[2] = data[3] = 0;
	write(dev_fnd,&data,4);	

	str_size = sizeof(fpga_number[0]);
	write(dev_dot, fpga_number[0], str_size);	

	memset(string, ' ', sizeof(string));
	write(dev_textlcd, string, 32);	

	close(dev_dot);
	close(dev_fnd);
	close(dev_textlcd);
};

void JNICALL Java_com_embe2020_project_MainActivity_controldotfnd(JNIEnv *env, jobject this, jint set_num, jint fnd_data){
	int str_size;
	int dev_dot;
	int dev_fnd;
	unsigned char data[4];

	dev_dot = open("/dev/fpga_dot", O_WRONLY);
	dev_fnd = open("/dev/fpga_fnd", O_WRONLY);

	data[3] = fnd_data%10; fnd_data /= 10;
	data[2] = fnd_data%10; fnd_data /= 10;
	data[1] = fnd_data%10; fnd_data /= 10;
	data[0] = fnd_data%10; 
	write(dev_fnd, &data, 4);	

	str_size = sizeof(fpga_number[set_num]);
	write(dev_dot, fpga_number[set_num], str_size);	

	close(dev_dot);
	close(dev_fnd);
};

JNIEXPORT void JNICALL Java_com_embe2020_project_MainActivity_controltextlcd(JNIEnv *env, jobject this, jstring speed_path, jstring amount_path){
	int dev_textlcd;
	int str_size;
	const char *speed;
	const char *amount;
	unsigned char string[32];

	dev_textlcd = open("/dev/fpga_text_lcd", O_WRONLY);
	memset(string, 0, sizeof(string));

	jboolean isCopy_s, isCopy_a;
	speed = (*env)->GetStringUTFChars(env, speed_path, &isCopy_s);
	amount = (*env)->GetStringUTFChars(env, amount_path, &isCopy_a);

	strncat(string, "Speed  : ", 9);
	str_size=strlen(speed);
	strncat(string, speed, str_size);
	memset(string+9+str_size, ' ', 16-9-str_size);

	strncat(string, "Amount : ", 9);
	str_size=strlen(amount);
	strncat(string, amount, str_size);
	memset(string+16+9+str_size, ' ', 16-9-str_size);

	write(dev_textlcd, string, 32);	

	if (isCopy_s == JNI_TRUE) {
		(*env)->ReleaseStringUTFChars(env, speed_path, speed);
	}
	if (isCopy_a == JNI_TRUE) {
		(*env)->ReleaseStringUTFChars(env, amount_path, amount);
	}
	
	close(dev_textlcd);
};