#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "gpio_test.h"
#include "sleng_debug.h"


#define DEFAULT_CONFIG_FILE_PATH	"./gpio.conf"


typedef struct gpio_config_result
{
	struct {
		int gpio;				/**< GPIO index */
		unsigned char out[2];		/**< Test result as output */
		unsigned char in[2];		/**< Test result as input */
	} pair[2];
} GPIO_CONFIG_RESULT, *PGPIO_CONFIG_RESULT;


int main(int argc, const char *argv[])
{
	int ret = 0;
	int fd;
	CDHX_DEV_PARAM dev;
	// int gpio,action,option;
	GPIO_CONFIG_RESULT gpio_matrix[10] = {0};
	const char *config = DEFAULT_CONFIG_FILE_PATH;
	FILE *fp_config = NULL;
	int i, count = 0;
	// const char short_options[] = "c:";

	do {
		/* Load config */
		if (argc > 1)
		{
			config = argv[1];
		}

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
			sleng_debug("GPIO[%d] - GPIO[%d]\n", gpio_matrix[i].pair[0].gpio, gpio_matrix[i].pair[1].gpio);
		}
		count = i;

		/* Do test */
		fd = open(GPIO_NODE, O_WRONLY);
		if (fd == -1)
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
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* Set [1] input */
			dev.action = 1;
			dev.option = 1;
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[0].gpio;			/* [0] output HIGH */
			dev.action = 6;
			dev.option = 1;
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* Read from [1] */
			dev.action = 5;
			dev.option = 1;
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}
			sleng_debug("option = %d\n", dev.option);
			gpio_matrix[i].pair[0].out[1] = gpio_matrix[i].pair[1].in[1] = dev.option;

			dev.gpio = gpio_matrix[i].pair[0].gpio;			/* [0] output LOW */
			dev.action = 6;
			dev.option = 0;
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* Read from [1] */
			dev.action = 5;
			dev.option = 1;
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}
			sleng_debug("option = %d\n", dev.option);
			gpio_matrix[i].pair[0].out[0] = gpio_matrix[i].pair[1].in[0] = dev.option;
			/* Print result */
			sleng_debug("GPIO[%d] -> GPIO[%d], change %d -> %d\n", gpio_matrix[i].pair[0].gpio, gpio_matrix[i].pair[1].gpio, gpio_matrix[i].pair[1].in[1], gpio_matrix[i].pair[1].in[0]);

			/* [1] out, [0] in */
			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* Set [1] output */
			dev.action = 2;
			dev.option = 1;
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[0].gpio;			/* Set [0] input */
			dev.action = 1;
			dev.option = 1;
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* [1] output HIGH */
			dev.action = 6;
			dev.option = 1;
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[0].gpio;			/* Read from [0] */
			dev.action = 5;
			dev.option = 1;
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}
			gpio_matrix[i].pair[1].out[1] = gpio_matrix[i].pair[0].in[1] = dev.option;

			dev.gpio = gpio_matrix[i].pair[1].gpio;			/* [1] output LOW */
			dev.action = 6;
			dev.option = 0;
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}

			dev.gpio = gpio_matrix[i].pair[0].gpio;			/* Read from [0] */
			dev.action = 5;
			dev.option = 1;
			if (ioctl(fd, CDHX_GPIO, &dev) < 0){
				sleng_error("error ioctl");
				ret = -1;
				break;
			}
			gpio_matrix[i].pair[1].out[0] = gpio_matrix[i].pair[0].in[0] = dev.option;

			/* Print result */
			// sleng_debug(" gpio test Success:gpio=%d,action=%d,option=%d\n", dev.gpio, dev.action, dev.option);
			sleng_debug("GPIO[%d] -> GPIO[%d], change %d -> %d\n", gpio_matrix[i].pair[1].gpio, gpio_matrix[i].pair[0].gpio, gpio_matrix[i].pair[0].in[1], gpio_matrix[i].pair[0].in[0]);
		}



		// }
	} while(0);

	/* Cleanup */
	close(fd);
	if (fp_config)
	{
		fclose(fp_config);
		fp_config = NULL;
	}

	return ret;
}
