#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <getopt.h>

#include "gpio_test.h"
#include "sleng_debug.h"


#define DEFAULT_CONFIG_FILE_PATH	"./gpio.conf"


typedef struct gpio_test_file_desc
{
	char input_file[64];
	int quit_flag: 1;
	int debug_flag: 1;
} GPIO_TEST_FD_S, *PGPIO_TEST_FD_S;
#define GPIO_TEST_FD_S_LEN  sizeof(GPIO_TEST_FD_S)
static GPIO_TEST_FD_S gpio_fd = {0};


typedef struct gpio_config_result
{
	struct {
		int gpio;					/**< GPIO index */
		unsigned char out[2];		/**< Test result as output */
		unsigned char in[2];		/**< Test result as input */
	} pair[2];
} GPIO_CONFIG_RESULT, *PGPIO_CONFIG_RESULT;


static inline void _print_devide_line(char *buf, int size, int offset)
{
	memset(buf, ' ', offset);
	memset(buf + offset, '-', size);
	buf[size - 1] = '\n';
	buf[size] = 0;
	fwrite(buf, 1, size, stdout);
}


static void _gpio_stat_print(GPIO_CONFIG_RESULT *gpio_matrix, int array_size)
{
	int i = 0;
	char buf[256] = {0,};

	printf("Statistics:\n");
	sprintf(buf, "    |   PIN    | OUT  |  IN  |\n");
	_print_devide_line(buf, strlen(buf), 4);
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "    |   PIN    | OUT  |  IN  |\n");
	fwrite(buf, 1, sizeof(buf), stdout);
	_print_devide_line(buf, strlen(buf), 4);
	for (i = 0; i < array_size; i++)
	{
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "    | [GPIO%2d] | %s | %s |\n", gpio_matrix[i].pair[0].gpio, (gpio_matrix[i].pair[1].in[0] == 0 && gpio_matrix[i].pair[1].in[1] == 1)? "\033[0;32;32mPass\033[m": "\033[0;32;31mFail\033[m", (gpio_matrix[i].pair[0].in[0] == 0 && gpio_matrix[i].pair[0].in[1] == 1)? "\033[0;32;32mPass\033[m": "\033[0;32;31mFail\033[m");
		fwrite(buf, 1, sizeof(buf), stdout);
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "    | [GPIO%2d] | %s | %s |\n", gpio_matrix[i].pair[1].gpio, (gpio_matrix[i].pair[0].in[0] == 0 && gpio_matrix[i].pair[0].in[1] == 1)? "\033[0;32;32mPass\033[m": "\033[0;32;31mFail\033[m", (gpio_matrix[i].pair[1].in[0] == 0 && gpio_matrix[i].pair[1].in[1] == 1)? "\033[0;32;32mPass\033[m": "\033[0;32;31mFail\033[m");
		fwrite(buf, 1, sizeof(buf), stdout);
	}
	_print_devide_line(buf, strlen(buf), 4);
}


int main(int argc, const char *argv[])
{
	PGPIO_TEST_FD_S fd = &gpio_fd;
	int ret = 0;
	int fd_gpio;
	CDHX_DEV_PARAM dev;
	// int gpio,action,option;
	GPIO_CONFIG_RESULT gpio_matrix[10] = {0};
	const char *config = DEFAULT_CONFIG_FILE_PATH;
	FILE *fp_config = NULL;
	int i, count = 0;
	// const char short_options[] = "c:";
	const char short_options[] = "c:d";
	const struct option long_options[] = {
		{"config",		required_argument,	NULL,	'c'},
		{"debug",		no_argument,		NULL,	'd'},
		{0, 0, 0, 0}
	};
	int opt, index;

	do {
		opt = getopt_long(argc, (char *const *)argv, short_options, long_options, &index);

		if (opt == -1) {
			break;
		}

		switch (opt) {
			case 'c': {
				config = optarg;
				break;
			}

			case 'd':{
				fd->debug_flag = 1;
				if (fd->debug_flag) sleng_debug("debug mode enable\n");
				break;
			}

			default : {
				if (fd->debug_flag) sleng_debug("Param(%c) is invalid\n", opt);
				break;
			}
		}
	} while (1);

	do {
		/* Load config */
		if ((fp_config = fopen(config, "r")) == NULL)
		{
			sleng_error("fopen[%s] error", config);
			ret = -1;
			break;
		}

		for (i = 0; i < 10; i++)
		{
			gpio_matrix[i].pair[0].gpio = gpio_matrix[i].pair[1].gpio = -1;
		}

		for (i = 0; i < 10; i++)
		{
			if (fscanf(fp_config, "GPIO%d GPIO%d\n", &gpio_matrix[i].pair[0].gpio, &gpio_matrix[i].pair[1].gpio) < 0)
			{
				break;
			}
			// if (fd->debug_flag)
			sleng_debug("GPIO[%d] - GPIO[%d]\n", gpio_matrix[i].pair[0].gpio, gpio_matrix[i].pair[1].gpio);
		}
		count = i;

		/* Do test */
		fd_gpio = open(GPIO_NODE, O_WRONLY);
		if (fd_gpio == -1)
		{
			sleng_error("error node");
			ret = -1;
			break;
		}

		for (i = 0; i < count; i++)
		{
			// sleng_debug("gpio test:gpio=%d,action=%d,option=%d\n", gpio, action, option);
			/* [0] out, [1] in */
			dev.gpio = gpio_matrix[i].pair[0].gpio;			/* Set [0] output */
			dev.action = 2;
			dev.option = 1;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* Set [1] input */
			dev.action = 1;
			dev.option = 1;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[0].gpio;			/* [0] output HIGH */
			dev.action = 6;
			dev.option = 1;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* Read from [1] */
			dev.action = 5;
			dev.option = 1;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}
			// sleng_debug("option = %d\n", dev.option);
			gpio_matrix[i].pair[0].out[1] = gpio_matrix[i].pair[1].in[1] = dev.option;

			dev.gpio = gpio_matrix[i].pair[0].gpio;			/* [0] output LOW */
			dev.action = 6;
			dev.option = 0;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* Read from [1] */
			dev.action = 5;
			dev.option = 1;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}
			// sleng_debug("option = %d\n", dev.option);
			gpio_matrix[i].pair[0].out[0] = gpio_matrix[i].pair[1].in[0] = dev.option;
			/* Print result */
			if (fd->debug_flag) sleng_debug("GPIO[%d] -> GPIO[%d], change %hhu -> %hhu\n", gpio_matrix[i].pair[0].gpio, gpio_matrix[i].pair[1].gpio, gpio_matrix[i].pair[1].in[1], gpio_matrix[i].pair[1].in[0]);

			/* [1] out, [0] in */
			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* Set [1] output */
			dev.action = 2;
			dev.option = 1;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[0].gpio;			/* Set [0] input */
			dev.action = 1;
			dev.option = 1;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* [1] output HIGH */
			dev.action = 6;
			dev.option = 1;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[0].gpio;			/* Read from [0] */
			dev.action = 5;
			dev.option = 1;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}
			gpio_matrix[i].pair[1].out[1] = gpio_matrix[i].pair[0].in[1] = dev.option;

			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* [1] output LOW */
			dev.action = 6;
			dev.option = 0;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[0].gpio;			/* Read from [0] */
			dev.action = 5;
			dev.option = 1;
			if (ioctl(fd_gpio, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}
			gpio_matrix[i].pair[1].out[0] = gpio_matrix[i].pair[0].in[0] = dev.option;

			/* Print result */
			// sleng_debug(" gpio test Success:gpio=%d,action=%d,option=%d\n", dev.gpio, dev.action, dev.option);
			// if (fd->debug_flag)
			// sleng_debug("GPIO[%d] -> GPIO[%d], change %hhu -> %hhu\n", gpio_matrix[i].pair[1].gpio, gpio_matrix[i].pair[0].gpio, gpio_matrix[i].pair[0].in[1], gpio_matrix[i].pair[0].in[0]);
		}

		_gpio_stat_print(gpio_matrix, count);
	} while(0);

	/* Cleanup */
	close(fd_gpio);
	if (fp_config)
	{
		fclose(fp_config);
		fp_config = NULL;
	}

	return ret;
}
