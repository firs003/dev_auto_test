#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "gpio_test.h"
#include "sleng_debug.h"

int main(int argc, const char *argv[])
{
	int fd;
	CDHX_DEV_PARAM dev;
	int gpio,action,option;

	fd = open(GPIO_NODE, O_WRONLY);
	if (fd == -1)
		printf("error node\n");

	gpio = atoi(argv[1]);
	action = atoi(argv[2]);
	option = atoi(argv[3]);

	sleng_debug("gpio test:gpio=%d,action=%d,option=%d\n", gpio, action, option);

	// for(; i < 20;i++){
	dev.gpio = gpio;
	dev.action = action;
	dev.option = option;
	if (ioctl(fd, CDHX_GPIO, &dev) < 0){
		printf("error ioctl\n");
		close(fd);
		return -1;
	}
	sleng_debug(" gpio test Success:gpio=%d,action=%d,option=%d\n", dev.gpio, dev.action, dev.option);
	// }
	close(fd);


	return 0;
}
