#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <libgen.h>
#include <time.h>

#include "sleng_debug.h"


#define DEFAULT_DEV_PATH		"/dev/ttymxc1"
#define	DEFAULT_BAUDRATE		115200
#define DEFAULT_DATA_BITS		8
#define DEFAULT_STOP_BITS		1
#define DEFAULT_PARITY_MODE		0
#define DEFAULT_FLOW_CTRL		0
// #define DEFAULT_INPUT_FILE	"/dev/zero"
#define DEFAULT_INPUT_FILE		NULL
#define DEFAULT_OUTPUT_FILE		"/dev/null"
#define DEFAULT_SENDBUF_SIZE	4096
#define DEFAULT_RECVBUF_SIZE	4096
#define DEFAULT_SEC_INTERVAL	1
#define DEFAULT_LAST_TIME		0xFFFFFFFF
#define DEFAULT_SEND_SPEED		1024
#define DEFAULT_PACKAGE_SIZE	32
#define CONST_LEN				32
#define FILE_ALLLOG_PATH		"all.log"
#define FILE_ERRLOG_PATH		"err.log"

int gquit_flag = 0;
int gdebug_flag = 0;
int err_count = 0;

static void signal_handler(int signo) {
	// if (gdebug_flag) sleng_debug("signo=%d, SIGPIPE=%d\n", __func__, signo, SIGPIPE);
	switch (signo) {
	case SIGINT:
	case SIGTERM:
		signal(signo, SIG_DFL);
		gquit_flag = 1;
		break;
	case SIGPIPE:
		signal(signo, SIG_IGN);
		break;
	default:
		signal(signo, SIG_DFL);
		break;
	}
	if (gdebug_flag) sleng_debug("signo=%d, gquit_flag=%d\n", signo, gquit_flag);
}

typedef enum serial_rw_mode {
	SERIAL_MODE_NONE = 0,
	SERIAL_MODE_RDONLY = 1,
	SERIAL_MODE_WRONLY = 2,
	SERIAL_MODE_RDWR = 3,
	SERIAL_MODE_MAX
} serial_rw_mode_e;

static int serial_open(const char *dev_path, int mode, int baudrate)
{
	int fd = -1, fd_mode = 0;
	unsigned int value;
	struct termios options;

	do {
	//	if (gdebug_flag) sleng_debug("master: baudrate = %u\n", baudrate);
		switch (baudrate) {
			case 300 :
				value = B300;
				break;
			case 600 :
				value = B600;
				break;
			case 1200 :
				value = B1200;
				break;
			case 2400 :
				value = B2400;
				break;
			case 4800 :
				value = B4800;
				break;
			case 9600 :
				value = B9600;
				break;
			case 19200 :
				value = B19200;
				break;
			case 38400 :
				value = B38400;
				break;
			case 57600 :
				value = B57600;
				break;
			case 115200 :
				value = B115200;
				break;
			// case 128000 :
			// 	value = B128000;
			// 	break;
			default :
				value = B115200;
				break;
		}

		switch (mode) {
			case SERIAL_MODE_RDONLY :
				fd_mode = O_RDONLY;
				break;
			case SERIAL_MODE_WRONLY :
				fd_mode = O_WRONLY;
				break;
			case SERIAL_MODE_RDWR :
				fd_mode = O_RDWR;
				break;
			default :
				fd_mode = O_RDONLY;
				break;
		}
		// if ((fd = open(dev_path, fd_mode | O_NOCTTY | O_NDELAY)) == -1) {
		if ((fd = open(dev_path, fd_mode | O_NOCTTY)) == -1) {
			sleng_error(":open serial device failure");
			break;
		}
		if (tcgetattr(fd, &options) < 0) {
			sleng_error(":tcgetattr failure");
			if (fd > 0) {
				close(fd);
				fd = -1;
			}
			break;
		}

		/* Set the baud rate to baudrate */
		if (mode & SERIAL_MODE_RDONLY && cfsetispeed(&options, value) < 0) {
			sleng_error(":cfsetispeed failure");
			if (fd > 0) {
				close(fd);
				fd = -1;
			}
			break;
		}
		if (mode & SERIAL_MODE_WRONLY && cfsetospeed(&options, value) < 0) {
			sleng_error(":cfsetospeed failure");
			if (fd > 0) {
				close(fd);
				fd = -1;
			}
			break;
		}

		/* Enable the receiver and set local mode...*/
		options.c_cflag |= (CLOCAL | CREAD);
		/* Set c_cflag options.*/
		// options.c_cflag |= PARENB;
		// options.c_cflag &= ~PARODD;
		// options.c_cflag &= ~CSTOPB;
		// options.c_cflag &= ~CSIZE;
		// options.c_cflag |= CS8;
		/* Set c_cflag options.*/
		options.c_cflag &= ~PARENB;	//disable parity bit
		options.c_cflag &= ~PARODD;	//PARODD, Use odd parity instead of even
		options.c_cflag &= ~CSTOPB;	//1 stop bits
		options.c_cflag &= ~CSIZE;
		options.c_cflag |= CS8;
		options.c_iflag &= ~(INPCK|ISTRIP);
		if (tcsetattr(fd, TCSANOW, &options) < 0) {
			sleng_error(":tcsetattr c_cflag failure");
			if (fd > 0) {
				close(fd);
				fd = -1;
			}
			break;
		}
		/* Set c_iflag input options */
		options.c_iflag &=~(IXON | IXOFF | IXANY);
		options.c_iflag &=~(INLCR | IGNCR | ICRNL);
		options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG | ECHONL | IEXTEN);

		/* Set c_oflag output options */
		options.c_oflag &= ~OPOST;

		/* Set the timeout options */
		// options.c_cc[VMIN]  = 0;
		// options.c_cc[VTIME] = 10;
		if (tcsetattr(fd, TCSANOW, &options) < 0) {
			sleng_error(":tcsetattr c_cc c_iflag c_lflag failure");
			if (fd > 0) {
				close(fd);
				fd = -1;
			}
			break;
		}
	} while (0);

	return fd;
}

typedef enum serial_buf_compare {
	SERIAL_BUF_COMPARE_IDENTICAL = 0,
	SERIAL_BUF_COMPARE_MISMATCH,
	SERIAL_BUF_COMPARE_CONNECT,
	SERIAL_BUF_COMPARE_KNOWN,
	SERIAL_BUF_COMPARE_MAX
} serial_buf_compare_e;

void log_compare(FILE *fp_alllog, int compare, char *dev, const char *recvbuf, int recvlen) {
	FILE *fp_errlog = NULL;
	FILE *fp_backup = NULL;
	// int fd_errlog;
	char msg[256] = {0,};
	char backup_path[256] = {0, };
	char *result_reason = NULL;
	struct timeval tv;
	struct tm *tmp;
	gettimeofday(&tv, NULL);
	tmp = localtime(&tv.tv_sec);

	switch (compare) {
		case SERIAL_BUF_COMPARE_IDENTICAL :
			result_reason = "Pass          ";
			break;
		case SERIAL_BUF_COMPARE_MISMATCH :
			result_reason = "Fail(mismatch)";
			break;
		case SERIAL_BUF_COMPARE_CONNECT :
			result_reason = "Fail(connect) ";
			break;
		// case SERIAL_BUF_COMPARE_KNOWN :
		// 	result_reason = "";
		// 	break;
		default :
			result_reason = "Fail(unknown) ";
			break;
	}
	// sprintf(backup_path, "%s_%d_%04d%02d%02d_%02d:%02d:%02d",
	// 		basename(dev), err_count,
	// 		tmp->tm_year+1900,tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	sprintf(backup_path, "mismatch/%s", basename(dev));
	sprintf(msg, "%04d/%02d/%02d %02d:%02d:%02d  %s  test:%s  err_count:%d\n",
			tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday, tmp->tm_hour, tmp->tm_min, tmp->tm_sec,
			basename(dev), result_reason, err_count);

	do {
		/* Write all log */
		if (flock(fileno(fp_alllog), LOCK_EX) == -1) {
			sleng_error("lock err_log_file[%s] failure", FILE_ALLLOG_PATH);
			break;
		}
		// sleng_debug("fwrite to all_log_file\n");
		if (fwrite(msg, 1, strlen(msg), fp_alllog) < 1) {
			sleng_error("fwrite to err_log_file[%s] failure", FILE_ALLLOG_PATH);
			break;
		}
		if (flock(fileno(fp_alllog), LOCK_UN) == -1) {
			sleng_error("unlock err_log_file[%s] failure", FILE_ALLLOG_PATH);
			break;
		}

		if (compare != SERIAL_BUF_COMPARE_IDENTICAL) {
			err_count++;
			/* Write err log */
			fp_errlog = fopen(FILE_ERRLOG_PATH, "a");
			if (!fp_errlog) {
				sleng_error("open err_log_file[%s] failure", FILE_ERRLOG_PATH);
				break;
			}
			fp_backup = fopen(backup_path, "a");
			if (!fp_errlog) {
				sleng_error("open backup_file[%s] failure", backup_path);
				break;
			}

			if (flock(fileno(fp_errlog), LOCK_EX) == -1) {
				sleng_error("lock err_log_file[%s] failure", FILE_ERRLOG_PATH);
				break;
			}
			if (fwrite(msg, 1, strlen(msg), fp_errlog) < 1) {
				sleng_error("fwrite to err_log_file[%s] failure", FILE_ERRLOG_PATH);
				break;
			}
			if (flock(fileno(fp_errlog), LOCK_UN) == -1) {
				sleng_error("unlock err_log_file[%s] failure", FILE_ERRLOG_PATH);
				break;
			}
			fclose(fp_errlog);
			fp_errlog = NULL;

			/* Backup err recv buffer */
			if (fwrite(msg, 1, strlen(msg), fp_backup) < 1) {
				sleng_error("fwrite head to backup_file[%s] failure", backup_path);
				break;
			}

			if (fwrite(recvbuf, 1, recvlen, fp_backup) < 1) {
				sleng_error("fwrite recv to backup_file[%s] failure", backup_path);
				break;
			}
			fprintf(fp_backup, "\n\n");
			fclose(fp_backup);
			fp_backup = NULL;
		}
	} while (0);

	if (fp_errlog) {
		fclose(fp_errlog);
		fp_errlog = NULL;
	}

	if (fp_backup) {
		fclose(fp_backup);
		fp_backup = NULL;
	}
}

/**************************************************
 * ./serial_test --dev /dev/ttymxc1 --mode send --output-file SComc1
 * ./serial_test --dev /dev/ttyVIZ0 --mode recv --output-file RComZ0
 **************************************************/
int main(int argc, char const *argv[]) {
	char *dev_path        = DEFAULT_DEV_PATH;
	int rw_mode           = SERIAL_MODE_RDONLY;
	int baudrate          = DEFAULT_BAUDRATE;
	int data_bits         = DEFAULT_DATA_BITS;
	int stop_bits         = DEFAULT_STOP_BITS;
	int parity_mode       = DEFAULT_PARITY_MODE;
	int flow_control      = DEFAULT_FLOW_CTRL;
	char *input_file      = DEFAULT_INPUT_FILE;
	char *output_file     = DEFAULT_OUTPUT_FILE;
	unsigned int interval = DEFAULT_SEC_INTERVAL;
	int send_speed        = DEFAULT_SEND_SPEED;
	unsigned int time     = DEFAULT_LAST_TIME;
	int package_size      = DEFAULT_PACKAGE_SIZE;
	int fd_serial         = -1;
	FILE *fp_input        = NULL;
	FILE *fp_output       = NULL;
	FILE *fp_alllog		  = NULL;
	char *sendbuf         = NULL;
	char *recvbuf         = NULL;
	char *filebuf         = NULL;
	char *compbuf         = NULL;

	const char short_options[] = "e:m:b:a:s:p:f:i:o:w:u:t:k:d";
	const struct option long_options[] = {
		{"dev",			required_argument,	NULL,	'e'},
		{"mode",		required_argument,	NULL,	'm'},
		{"baudrate",	required_argument,	NULL,	'b'},
		{"data-bits", 	required_argument,	NULL,	'a'},
		{"stop-bits",	required_argument,	NULL,	's'},
		{"parity",		required_argument,	NULL,	'p'},
		{"flow-control",required_argument,	NULL,	'f'},
		{"input-file",	required_argument,	NULL,	'i'},
		{"output-file",	required_argument,	NULL,	'o'},
		{"send-speed",	required_argument,	NULL,	'w'},
		{"pack-size",	required_argument,	NULL,	'k'},
		{"interval",	required_argument,	NULL,	'u'},
		{"time",		required_argument,	NULL,	't'},
		{"debug",		no_argument,		NULL,	'd'},
		{0, 0, 0, 0}
	};
	int opt, index;

	signal(SIGINT, signal_handler);

	do {
		opt = getopt_long(argc, (char *const *)argv, short_options, long_options, &index);

		if (opt == -1) {
			break;
		}

		switch (opt) {
			case 'e': {
				dev_path = optarg;
				break;
			}
			case 'm': {
				if (strcmp(optarg, "w") == 0 || strcmp(optarg, "wr") == 0 || strcmp(optarg, "write") == 0 || strcmp(optarg, "s") == 0 || strcmp(optarg, "send") == 0) {
					rw_mode = SERIAL_MODE_WRONLY;
				}
				if (strcmp(optarg, "rw") == 0 || strcmp(optarg, "rs") == 0 || strcmp(optarg, "wr") == 0  || strcmp(optarg, "sr") == 0) {
					rw_mode = SERIAL_MODE_RDWR;
				}
				break;
			}
			case 'b': {
				baudrate = atoi(optarg);
				break;
			}

			case 'a': {
				data_bits = atoi(optarg);
				if (gdebug_flag) sleng_debug(":data_bits=%d\n", data_bits);
				break;
			}

			case 's': {
				stop_bits = atoi(optarg);
				if (gdebug_flag) sleng_debug(":stop_bits=%d\n", stop_bits);
				break;
			}

			case 'p': {
				parity_mode = atoi(optarg);
				if (gdebug_flag) sleng_debug(":parity_mode=%d\n", parity_mode);
				break;
			}

			case 'f': {
				flow_control = atoi(optarg);
				if (gdebug_flag) sleng_debug(":flow_control=%d\n", flow_control);
				break;
			}

			case 'i': {
				input_file = optarg;
				break;
			}

			case 'o': {
				output_file = optarg;
				break;
			}

			case 'w': {
				send_speed = (atoi(optarg)%package_size)? (atoi(optarg)/package_size + 1) * package_size: atoi(optarg);
				break;
			}

			case 'k': {
				// package_size = (atoi(optarg)%32)? (atoi(optarg)/32 + 1) * 32: atoi(optarg);
				package_size = atoi(optarg);
				break;
			}

			case 'u': {
				interval = atoi(optarg);
				if (gdebug_flag) sleng_debug("get interval=%d, args=%s\n", interval, optarg);
				break;
			}

			case 't': {
				time = atol(optarg);
				if (gdebug_flag) sleng_debug("time=%d\n", time);
				break;
			}

			case 'd':{
				gdebug_flag = 1;
				if (gdebug_flag) sleng_debug("debug mode enable\n");
				break;
			}

			default : {
				if (gdebug_flag) sleng_debug("Param(%c) is invalid\n", opt);
				break;
			}
		}
	} while (1);

	if (gdebug_flag) sleng_debug("[serial:%s] %d %d %s %d(%s), input=%s, output=%s, speed=%d, interval=%ds\n",
		dev_path, baudrate, data_bits, (parity_mode)? "Y": "N", stop_bits, (flow_control)? "enable": "none",
		(input_file)? input_file: "(nil)", output_file, send_speed, interval);
	do {
		int i;
		fp_alllog = fopen(FILE_ALLLOG_PATH, "a");
		if (!fp_alllog) {
			sleng_error("open all_log_file[%s] failure", FILE_ALLLOG_PATH);
			break;
		}
		if (rw_mode & SERIAL_MODE_WRONLY) {
			sendbuf = (char *)calloc(send_speed + 1, 1);
			if (!sendbuf) {
				sleng_error(":calloc for sendbuf failure");
				break;
			}
			if (input_file) {	//recv mode do NOT need input_file
				fp_input = fopen(input_file, "r");
				if (!fp_input) {
					sleng_error(":fopen input_file[%s] failure", input_file);
					break;
				}
			} else {
				char *p = sendbuf;
				char *tmp = "abcdefghijklmnopqrstuvwxyz1234567890\n";
				int leftlen = send_speed;
				while (leftlen > 0) {
					int cplen = (leftlen > strlen(tmp))? strlen(tmp): leftlen;
					memcpy(p, tmp, cplen);
					p += cplen;
					leftlen -= cplen;
				}
				sendbuf[send_speed - 1] = '\n';
			}
		}
		if (rw_mode & SERIAL_MODE_RDONLY) {
			recvbuf = (char *)calloc(send_speed + 1, 1);
			if (!recvbuf) {
				sleng_error(":calloc for recvbuf failure");
				break;
			}

			compbuf = (char *)calloc(send_speed + 1, 1);
			if (!compbuf) {
				sleng_error(":calloc for compbuf failure");
				break;
			}

			if (input_file) {	//recv mode do NOT need input_file
				fp_input = fopen(input_file, "r");
				if (!fp_input) {
					sleng_error(":fopen input_file[%s] failure", input_file);
					break;
				}
			} else {
				char *p = compbuf;
				char *tmp = "abcdefghijklmnopqrstuvwxyz1234567890\n";
				int leftlen = send_speed;
				while (leftlen > 0) {
					int cplen = (leftlen > strlen(tmp))? strlen(tmp): leftlen;
					memcpy(p, tmp, cplen);
					p += cplen;
					leftlen -= cplen;
				}
				compbuf[send_speed - 1] = '\n';
			}
		}

		fd_serial = serial_open(dev_path, rw_mode, baudrate);
		if (fd_serial == -1) {
			sleng_error(":serial_open failure");
			break;
		} else {
			if (gdebug_flag) sleng_debug(":serial_open success, fd_serial=%d\n", fd_serial);
		}

		fp_output = fopen(output_file, "w");
		if (!fp_output) {
			sleng_error(":fopen output_file[%s] failure", output_file);
			break;
		}

		// while (!gquit_flag) {
		for(i=0; !gquit_flag && i<time; i++) {
			int err_flag = 0;
			/* Send Mode */
			if (rw_mode & SERIAL_MODE_WRONLY) {
				int leftlen = send_speed;
				char *p = sendbuf;
				/* Read from input file / or Generate data */
				if (fp_input) {

				} else {

				}
				/* Write to serial port */

				while (leftlen > 0) {
					int len = write(fd_serial, p, package_size);
					if (gdebug_flag) sleng_debug(":leftlen=%d, sendlen=%d\n", leftlen, len);
					if (len < 0) {
						sleng_error(":write fd_serial failure");
						err_flag = 1;
						break;
					}
					p += len;
					leftlen -= len;
				}
				// if (gdebug_flag) sleng_debug("send:\n%s\n", sendbuf);
				filebuf = sendbuf;
				if (err_flag) break;
				sleep(interval);
			}

			/* Recv Mode */
			if (rw_mode & SERIAL_MODE_RDONLY) {
				int compare = SERIAL_BUF_COMPARE_IDENTICAL;
				int leftlen = send_speed;
				char *p = recvbuf;
				memset(recvbuf, 0, send_speed);
				/* Read from specified serial port */
				while (leftlen > 0) {
					int len = read(fd_serial, p, package_size);
					if (gdebug_flag) sleng_debug(":leftlen=%d, recvlen=%d\n", leftlen, len);
					if (len < 0) {
						sleng_error(":read fd_serial failure");
						err_flag = 1;
						compare = SERIAL_BUF_COMPARE_CONNECT;
						break;
					}
					p += len;
					leftlen -= len;
				}
				if (fp_input) {

				} else {

				}
				if (memcmp(compbuf, recvbuf, send_speed)) {
					sleng_debug("[%s]Diff between recv and send!\n", basename(dev_path));
					compare = SERIAL_BUF_COMPARE_MISMATCH;
					// sleng_debug("recv:\n%s\n", recvbuf);
				}
				log_compare(fp_alllog, compare, dev_path, recvbuf, send_speed);
				if (gdebug_flag) sleng_debug("recv:\n%s\n", recvbuf);
				filebuf = recvbuf;
				if (err_flag) break;
			}
			/* Write to output file, for both RDONLY and WRONLY*/
			if (fwrite(filebuf, 1, send_speed, fp_output) < 0) {
				sleng_error(":fwrite fp_output[%s] failure", output_file);
				break;
			}
		}
	} while (0);


	if (fp_input) {
		fclose(fp_input);
		fp_input = NULL;
	}

	if (fp_output) {
		fclose(fp_output);
		fp_output = NULL;
	}

	if (fp_alllog) {
		fclose(fp_alllog);
		fp_alllog = NULL;
	}

	if (sendbuf) {
		free(sendbuf);
		sendbuf = NULL;
	}

	if (recvbuf) {
		free(recvbuf);
		recvbuf = NULL;
	}

	if (fd_serial > 0) {
		close(fd_serial);
		fd_serial = -1;
	}

	if (gdebug_flag) sleng_debug("[serial:%s] %s quit\n", dev_path, (rw_mode == SERIAL_MODE_RDONLY)? "recv": "send");
	return 0;
}
