#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <termios.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/select.h>

#include "sleng_debug.h"


#define DEFAULT_CONFIG_PATH     "./uart.conf"
#define DEFAULT_INPUT_FILE		"/disthen/ocm_modbus/ocm_modbus_run"
#define	DEFAULT_BAUDRATE		115200
#define DEFAULT_DATA_BITS		8
#define DEFAULT_STOP_BITS		1
#define DEFAULT_PARITY_MODE		0
#define DEFAULT_FLOW_CTRL		0
#define MAX_UART_AMOUNT         8

#define THREAD_SUCCESS  ((void *)0)
#define THREAD_FAILURE  ((void *)-1)


typedef enum thread_role
{
    THREAD_ROLE_SENDER,
    THREAD_ROLE_RECVER,
} THREAD_ROLE_E;


typedef struct uart_config_s
{
    int role:1;
    char uart_path[32];
    pthread_t tid;
    int packsize;
    int baudrate;
    unsigned int sendcnt;
    unsigned int senderr;
    unsigned int recvcnt;
    unsigned int recverr;
} UART_CONFIG_S, *PUART_CONFIG_S;
#define UART_CONFIG_S_LEN   sizeof(UART_CONFIG_S)


#if 0
typedef struct send_thread_arg
{

} SEND_THREAD_ARG_S, *PSEND_THREAD_ARG_S;
#define SEND_THREAD_ARG_S_LEN   sizeof(SEND_THREAD_ARG_S)


typedef struct recv_thread_arg
{

} RECV_THREAD_ARG_S, *PRECV_THREAD_ARG_S;
#define RECV_THREAD_ARG_S_LEN   sizeof(RECV_THREAD_ARG_S)
#endif


typedef struct uart_test_file_desc
{
    char input_file[64];
    int quit_flag: 1;
    int debug_flag: 1;
} UART_TEST_FD_S, *PUART_TEST_FD_S;
#define UART_TEST_FD_S_LEN  sizeof(UART_TEST_FD_S)
static UART_TEST_FD_S uart_fd = {0};


typedef enum uart_rw_mode {
    UART_MODE_NONE = 0,
    UART_MODE_RDONLY = 1,
    UART_MODE_WRONLY = 2,
    UART_MODE_RDWR = 3,
    UART_MODE_MAX
} uart_rw_mode_e;



static void signal_handler(int signo) {
    // if (fd->debug_flag) sleng_debug("signo=%d, SIGPIPE=%d\n", __func__, signo, SIGPIPE);
    PUART_TEST_FD_S fd = &uart_fd;
    switch (signo) {
    case SIGINT:
    case SIGTERM:
        signal(signo, SIG_DFL);
        fd->quit_flag = 1;
        break;
    case SIGPIPE:
        signal(signo, SIG_IGN);
        break;
    default:
        signal(signo, SIG_DFL);
        break;
    }
    if (fd->debug_flag) sleng_debug("signo=%d, fd->quit_flag=%d\n", signo, fd->quit_flag);
}


static int uart_open(const char *dev_path, int mode, int baudrate)
{
    // PUART_TEST_FD_S fd = &uart_fd;
    int fd_uart = -1, fd_mode = 0;
    unsigned int value;
    struct termios options;

    do {
    	// if (fd->debug_flag) sleng_debug("baudrate = %u\n", baudrate);
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
            case UART_MODE_RDONLY :
                fd_mode = O_RDONLY;
                break;
            case UART_MODE_WRONLY :
                fd_mode = O_WRONLY;
                break;
            case UART_MODE_RDWR :
                fd_mode = O_RDWR;
                break;
            default :
                fd_mode = O_RDONLY;
                break;
        }
        // if ((fd_uart = open(dev_path, fd_mode | O_NOCTTY | O_NDELAY)) == -1) {
        if ((fd_uart = open(dev_path, fd_mode | O_NOCTTY)) == -1) {
            sleng_error(":open uart device failure");
            break;
        }
        if (tcgetattr(fd_uart, &options) < 0) {
            sleng_error(":tcgetattr failure");
            if (fd_uart > 0) {
                close(fd_uart);
                fd_uart = -1;
            }
            break;
        }

        /* Set the baud rate to baudrate */
        if (mode & UART_MODE_RDONLY && cfsetispeed(&options, value) < 0) {
            sleng_error(":cfsetispeed failure");
            if (fd_uart > 0) {
                close(fd_uart);
                fd_uart = -1;
            }
            break;
        }
        if (mode & UART_MODE_WRONLY && cfsetospeed(&options, value) < 0) {
            sleng_error(":cfsetospeed failure");
            if (fd_uart > 0) {
                close(fd_uart);
                fd_uart = -1;
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
        if (tcsetattr(fd_uart, TCSANOW, &options) < 0) {
            sleng_error(":tcsetattr c_cflag failure");
            if (fd_uart > 0) {
                close(fd_uart);
                fd_uart = -1;
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
        options.c_cc[VMIN]  = 0;
        options.c_cc[VTIME] = 10;
        if (tcsetattr(fd_uart, TCSANOW, &options) < 0) {
            sleng_error(":tcsetattr c_cc c_iflag c_lflag failure");
            if (fd_uart > 0) {
                close(fd_uart);
                fd_uart = -1;
            }
            break;
        }
    } while (0);

    return fd_uart;
}


void *send_thread_func(void *arg)
{
    PUART_TEST_FD_S fd = &uart_fd;
    void *ret = THREAD_SUCCESS;
    PUART_CONFIG_S args = (PUART_CONFIG_S)arg;
    int fd_uart = -1;
    int readlen = 0, recvlen = 0, sendlen = 0;
    char *recvbuf = NULL;
    char *sendbuf = NULL;
    FILE *fp_input = NULL;
    fd_set rset;
    struct timeval tv;
    int rc = 0;

    do {
        fd_uart = uart_open(args->uart_path, UART_MODE_RDWR, args->baudrate);
        if (fd_uart == -1) {
            sleng_error("uart_open failure");
            ret = THREAD_FAILURE;
            break;
        }
        if (fd->debug_flag) sleng_debug("uart_open success, fd_uart=%d\n", fd_uart);

        recvbuf = malloc(args->packsize);
        if (recvbuf == NULL)
        {
            sleng_error("malloc for recvbuf failure");
            ret = THREAD_FAILURE;
            break;
        }
        if (fd->debug_flag) sleng_debug("malloc for recvbuf success, recvbuf[%d]=%p\n", args->packsize, recvbuf);

        sendbuf = malloc(args->packsize);
        if (sendbuf == NULL)
        {
            sleng_error("malloc for sendbuf failure");
            ret = THREAD_FAILURE;
            break;
        }
        if (fd->debug_flag) sleng_debug("malloc for sendbuf success, sendbuf[%d]=%p\n", args->packsize, sendbuf);

        fp_input = fopen(fd->input_file, "r");
        if (fp_input == NULL)
        {
            sleng_error("fopen [%s] as input failure", fd->input_file);
            ret = THREAD_FAILURE;
            break;
        }

        int i;
        for (i = 0; i < args->packsize; i++)
        {
            sendbuf[i] = i;
        }

        while (!fd->quit_flag)
        {
#if 0
            readlen = fread(sendbuf, 1, args->packsize, fp_input);
            if (readlen < 0)
            {
                sleng_error("fread from input[%s] error", fd->input_file);
                ret = THREAD_FAILURE;
                break;
            }
            if (readlen >= 0 && readlen < args->packsize)
            {
                fseek(fp_input, 0, SEEK_SET);
            }
#endif
            readlen = args->packsize;
            args->sendcnt++;
            sendlen = write(fd_uart, sendbuf, readlen);
            if (sendlen < recvlen)
            {
                args->senderr++;
                sleng_error("[%s] send[%d < %d] error, %u/%u, %.02f%%, 0x%02hhx", args->uart_path, sendlen, args->packsize, args->senderr, args->sendcnt, (args->sendcnt) ? 100 * (float)args->senderr / (float)args->sendcnt : 0.0, sendbuf[0]);
            }
            if (fd->debug_flag) sleng_debug("[%s]%u.sendlen=%d, buf=0x%02hhx %02hhx %02hhx %02hhx   %02hhx %02hhx %02hhx %02hhx\n", args->uart_path, args->sendcnt, sendlen, sendbuf[0], sendbuf[1], sendbuf[2], sendbuf[3], sendbuf[4], sendbuf[5], sendbuf[6], sendbuf[7]);

            args->recvcnt++;
            FD_ZERO(&rset);
            FD_SET(fd_uart, &rset);
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            rc = select(fd_uart + 1, &rset, NULL, NULL, &tv);
            // sleng_debug("select rc=%d\n", rc);
            if (rc < 0)
            {
                args->recverr++;
                sleng_error("[%s] select error, %u/%u, %.02f%%, 0x%02hhx", args->uart_path, args->recverr, args->recvcnt, (args->recvcnt) ? 100 * (float)args->recverr / (float)args->recvcnt : 0.0, recvbuf[0]);
                continue;
            }
            else if (rc == 0)
            {
                if (fd->quit_flag)
                {
                    args->recverr++;
                    sleng_error("[%s] select timeout, %u/%u, %.02f%%, 0x%02hhx", args->uart_path, args->recverr, args->recvcnt, (args->recvcnt) ? 100 * (float)args->recverr / (float)args->recvcnt : 0.0, recvbuf[0]);
                }
                else
                {
                    args->recvcnt--;
                }
                continue;
            }
            usleep(150000);
            memset(recvbuf, 0, args->packsize);
            recvlen = read(fd_uart, recvbuf, args->packsize);
            if (recvlen < args->packsize || memcmp(sendbuf, recvbuf, readlen))
            {
                args->recverr++;
                sleng_error("[%s] recv[%d < %d] error, %u/%u, %.02f%%, 0x%02hhx", args->uart_path, recvlen, args->packsize, args->recverr, args->recvcnt, (args->recvcnt) ? 100 * (float)args->recverr / (float)args->recvcnt : 0.0, recvbuf[0]);
            }
            // if (fd->debug_flag) sleng_debug("%u.recvlen=%d, buf=0x%02hhx %02hhx %02hhx %02hhx   %02hhx %02hhx %02hhx %02hhx\n", args->recvcnt, recvlen, recvbuf[0], recvbuf[1], recvbuf[2], recvbuf[3], recvbuf[4], recvbuf[5], recvbuf[6], recvbuf[7]);
            usleep(850000);
        }
    } while(0);

    if (fd_uart > 0)
    {
        close(fd_uart);
        fd_uart = -1;
    }
    if (sendbuf)
    {
        free(sendbuf);
        sendbuf = NULL;
    }
    if (recvbuf)
    {
        free(recvbuf);
        recvbuf = NULL;
    }
    if (fp_input)
    {
        fclose(fp_input);
        fp_input = NULL;
    }

    return ret;
}


void *recv_thread_func(void *arg)
{
    PUART_TEST_FD_S fd = &uart_fd;
    void *ret = THREAD_SUCCESS;
    PUART_CONFIG_S args = (PUART_CONFIG_S)arg;
    int fd_uart = -1;
    int recvlen = 0, sendlen = 0;
    char *recvbuf = NULL;
    fd_set rset;
    struct timeval tv;
    int rc = 0;

    fd->debug_flag = 1;

    do {
        fd_uart = uart_open(args->uart_path, UART_MODE_RDWR, args->baudrate);
        if (fd_uart == -1) {
            sleng_error("uart_open failure");
            ret = THREAD_FAILURE;
            break;
        }
        if (fd->debug_flag) sleng_debug("uart_open success, fd_uart=%d\n", fd_uart);

        recvbuf = malloc(args->packsize);
        if (recvbuf == NULL)
        {
            sleng_error("malloc for recvbuf failure");
            ret = THREAD_FAILURE;
            break;
        }
        if (fd->debug_flag) sleng_debug("malloc for recvbuf success, recvbuf[%d]=%p\n", args->packsize, recvbuf);

        while (!fd->quit_flag)
        {
            args->recvcnt++;
            FD_ZERO(&rset);
            FD_SET(fd_uart, &rset);
            tv.tv_sec = 2;
            tv.tv_usec = 0;
            rc = select(fd_uart + 1, &rset, NULL, NULL, &tv);
            if (rc < 0)
            {
                args->recverr++;
                sleng_error("[%s] select error, %u/%u, %.02f%%, 0x%02hhx", args->uart_path, args->recverr, args->recvcnt, (args->recvcnt) ? 100 * (float)args->recverr / (float)args->recvcnt : 0.0, recvbuf[0]);
                continue;
            }
            else if (rc == 0)
            {
                if (!fd->quit_flag)
                {
                    args->recverr++;
                    sleng_error("[%s] select timeout, %u/%u, %.02f%%, 0x%02hhx", args->uart_path, args->recverr, args->recvcnt, (args->recvcnt) ? 100 * (float)args->recverr / (float)args->recvcnt : 0.0, recvbuf[0]);
                }
                else
                {
                    args->recvcnt--;
                }
                continue;
            }
            usleep(150000);

            memset(recvbuf, 0, args->packsize);
            recvlen = read(fd_uart, recvbuf, args->packsize);
            if (recvlen < args->packsize)
            {
                args->recverr++;
                sleng_error("[%s] recv[%d < %d] error, %u/%u, %.02f%%, 0x%02hhx", args->uart_path, recvlen, args->packsize, args->recverr, args->recvcnt, (args->recvcnt) ? 100 * (float)args->recverr / (float)args->recvcnt : 0.0, recvbuf[0]);
            }
            if (fd->debug_flag) sleng_debug("[%s]%u.recvlen=%d, buf=0x%02hhx %02hhx %02hhx %02hhx   %02hhx %02hhx %02hhx %02hhx\n", args->uart_path, args->recvcnt, recvlen, recvbuf[0], recvbuf[1], recvbuf[2], recvbuf[3], recvbuf[4], recvbuf[5], recvbuf[6], recvbuf[7]);

            args->sendcnt++;
            sendlen = write(fd_uart, recvbuf, recvlen);
            if (sendlen < recvlen)
            {
                args->senderr++;
                sleng_error("[%s] send error, %u/%u, %.02f%%", args->uart_path, args->senderr, args->sendcnt, (args->sendcnt) ? 100 * (float)args->senderr / (float)args->sendcnt : 0.0);
            }
        }
    } while(0);

    if (fd_uart > 0)
    {
        close(fd_uart);
        fd_uart = -1;
    }
    if (recvbuf)
    {
        free(recvbuf);
        recvbuf = NULL;
    }

    return ret;
}


static inline void _print_devide_line(char *buf, int size, int offset)
{
    memset(buf, ' ', offset);
    memset(buf + offset, '-', size);
    buf[size - 1] = '\n';
    buf[size] = 0;
    fwrite(buf, 1, size, stdout);
}


static void _stat_print(UART_CONFIG_S *uart_config_array, int array_size)
{
    int i = 0;
    char buf[128] = {0,};

#if 0
    _print_devide_line(buf, sizeof(buf));
    memset(buf, 0, sizeof(buf));
    sprintf(buf, "    |No.| Dev\t| Total\t| Send\t| Recv\t|\n");
    fwrite(buf, 1, sizeof(buf), stdout);
#endif
    printf("Statistics:\n");
    sprintf(buf, "    | %d | %s | Total:%u | Send:%u - %3u%%, Fail:%u - %3u%% | Recv:%u - %3u%%, Fail:%u - %3u%% |\n", i, uart_config_array[i].uart_path, uart_config_array[i].sendcnt,
            uart_config_array[i].sendcnt - uart_config_array[i].senderr, (uart_config_array[i].sendcnt)? (uart_config_array[i].sendcnt - uart_config_array[i].senderr) * 100 / uart_config_array[i].sendcnt: 0,
            uart_config_array[i].senderr, (uart_config_array[i].sendcnt)? (uart_config_array[i].senderr) * 100 / uart_config_array[i].sendcnt: 0,
            uart_config_array[i].recvcnt - uart_config_array[i].recverr, (uart_config_array[i].recvcnt)? (uart_config_array[i].recvcnt - uart_config_array[i].recverr) * 100 / uart_config_array[i].recvcnt: 0,
            uart_config_array[i].recverr, (uart_config_array[i].recvcnt)? (uart_config_array[i].recverr) * 100 / uart_config_array[i].recvcnt: 0);
    _print_devide_line(buf, strlen(buf), 4);
    for (i = 0; i < array_size; i++)
    {
        // sleng_debug("path=%s, tid=%lu\n", uart_config_array[i].uart_path, uart_config_array[i].tid);
        if (uart_config_array[i].uart_path[0] != 0 && uart_config_array[i].tid > 0)
        {
            memset(buf, 0, sizeof(buf));
            sprintf(buf, "    | %d | %s | Total:%u | Send:%u - %3u%%, Fail:%u - %3u%% | Recv:%u - %3u%%, Fail:%u - %3u%% |\n", i, uart_config_array[i].uart_path, uart_config_array[i].sendcnt,
                    uart_config_array[i].sendcnt - uart_config_array[i].senderr, (uart_config_array[i].sendcnt)? (uart_config_array[i].sendcnt - uart_config_array[i].senderr) * 100 / uart_config_array[i].sendcnt: 0,
                    uart_config_array[i].senderr, (uart_config_array[i].sendcnt)? (uart_config_array[i].senderr) * 100 / uart_config_array[i].sendcnt: 0,
                    uart_config_array[i].recvcnt - uart_config_array[i].recverr, (uart_config_array[i].recvcnt)? (uart_config_array[i].recvcnt - uart_config_array[i].recverr) * 100 / uart_config_array[i].recvcnt: 0,
                    uart_config_array[i].recverr, (uart_config_array[i].recvcnt)? (uart_config_array[i].recverr) * 100 / uart_config_array[i].recvcnt: 0);
            fwrite(buf, 1, sizeof(buf), stdout);
        }
    }
    _print_devide_line(buf, strlen(buf), 4);
}


int main(int argc, char const *argv[])
{
    PUART_TEST_FD_S fd = &uart_fd;
    int ret = 0;
    void *thread_ret = THREAD_FAILURE;
    const char *config = DEFAULT_CONFIG_PATH;
    FILE *fp_config = NULL;
    UART_CONFIG_S uart_config_array[MAX_UART_AMOUNT] = {0};
    int i;
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
        signal(SIGINT, signal_handler);

        /* Load config */
        if ((fp_config = fopen(config, "r")) == NULL)
        {
            sleng_error("fopen[%s] error", config);
            ret = -1;
            break;
        }

        if (fscanf(fp_config, "input:%s\n", fd->input_file) < 1)
        {
            strncpy(fd->input_file, DEFAULT_INPUT_FILE, sizeof(fd->input_file));
            fseek(fp_config, 0, SEEK_SET);
        }

        for (i = 0; i < MAX_UART_AMOUNT; i += 2)
        {
            int packsize, baudrate;
            if (fscanf(fp_config, "send=%s recv=%s pack=%d baud=%d\n", uart_config_array[i].uart_path, uart_config_array[i + 1].uart_path, &packsize, &baudrate) < 0)
            {
                ret = -1;
                break;
            }
            // sleng_debug("send=%s recv=%s pack=%d baud=%d\n", uart_config_array[i].uart_path, uart_config_array[i + 1].uart_path, packsize, baudrate);
            uart_config_array[i].role     = THREAD_ROLE_SENDER;
            uart_config_array[i + 1].role = THREAD_ROLE_RECVER;
            uart_config_array[i].packsize = uart_config_array[i + 1].packsize = packsize;
            uart_config_array[i].baudrate = uart_config_array[i + 1].baudrate = baudrate;
            sleng_debug("UART[%s] ==> UART[%s]: %d bps, pack=%d\n", uart_config_array[i].uart_path, uart_config_array[i + 1].uart_path, uart_config_array[i].baudrate, uart_config_array[i].packsize);
        }
        // if (ret == -1)  break;

        for (i = 0; i < MAX_UART_AMOUNT; i++)
        {
            if (uart_config_array[i].uart_path[0] != 0)
            {
                if (pthread_create(&uart_config_array[i].tid, NULL, (uart_config_array[i].role == THREAD_ROLE_SENDER)? send_thread_func: recv_thread_func, (void *)&uart_config_array[i]) < 0)
                {
                    ret = -1;
                    break;
                }
                sleng_debug("%d.create %s[%lu] success\n", i, (uart_config_array[i].role == THREAD_ROLE_SENDER)? "sender": "recver", uart_config_array[i].tid);
            }
        }

    } while(0);

    for (i = 0; i < MAX_UART_AMOUNT; i++)
    {
        // sleng_debug("tid=%lu\n", uart_config_array[i].tid);
        if (uart_config_array[i].uart_path[0] != 0 && uart_config_array[i].tid > 0)
        {
            pthread_join(uart_config_array[i].tid, &thread_ret);
            sleng_warning("thread(%lu) return %s\n", uart_config_array[i].tid, thread_ret? "failure": "success");
        }
    }

    // _statistics_print(uart_config_array);
    _stat_print(uart_config_array, MAX_UART_AMOUNT);

    if (fp_config)
    {
        fclose(fp_config);
        fp_config = NULL;
    }

    return ret;
}
