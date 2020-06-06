Driver Name : /dev/stopwatch
Major Number : 242

(On host)
1. cd app
2. make
3. adb push app [Target Board Directory]
4. cd ../module
5. make
6. adb push dev_module.ko [Target Board Directory]

(On target)
7. insmod dev_driver.ko
8. mknod /dev/dev_driver c 242 0
9. ./app 

10. rmmod dev_driver