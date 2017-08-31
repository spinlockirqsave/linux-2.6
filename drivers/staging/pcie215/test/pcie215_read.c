#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <sys/mman.h>
#include <getopt.h>
#include <sys/time.h>


#include "../pcie215_ioctl.h"


#define PCIE215	"/dev/pcie215"	/* character device file */


void usage(char *name)
{
	if (name == NULL)
		return;

	fprintf(stderr, "\nusage:\t %s --help\n\n", name);
	fprintf(stderr, "     \t No arguments. Program will call \n");
	fprintf(stderr, "     \t \tfd = open(PCIE215, O_RDWR);\n");
	fprintf(stderr, "     \t \tres = read(fd, NULL, 0);\n");
	fprintf(stderr, "     \t and will be blocked waiting for the interrupt on any\n");
	fprintf(stderr, "     \t of the enabled pins (use pcie215_ioctl program to configure them).\n");
	fprintf(stderr, "\n");
	exit(EXIT_FAILURE);
}

static void timer_callback(int sig)
{
	fprintf(stderr, "Signal delivered to timer callback [%d] [%s].\n",
		sig, strsignal(sig));
}

void init_timer()
{
	struct sigaction sa;

	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_NODEFER;
	sa.sa_handler = &timer_callback;
	sigaction(SIGALRM, &sa, NULL);
}


int main(int argc, char *argv[])
{
	int c = 0;
	int fd = -1;										/* character device file descriptor */

	int res = -1;

	if (argc > 1)
		usage(argv[0]);

	int option_index = 0;
	struct option long_options[] = {
		{ "help", no_argument, 0, 1 },
		{ 0 }
	};


	while ((c = getopt_long(argc, argv, "", long_options, &option_index)) != -1) {		/* parse optional command line arguments */

		switch (c) {

		case 1:
			usage(argv[0]);								/* --help */
			break;

		case '?':
			if (optopt == 'r')
				fprintf(stderr, "Option -%c requires an argument.\n", optopt);
			else if (isprint(optopt))
				fprintf(stderr,"Unknown option '-%c'.\n", optopt);
			else {
				fprintf(stderr, "Are there any long options? "
					"Please check that you have typed them correctly.\n");
			}
			usage(argv[0]);

		default:
			exit(EXIT_FAILURE);
		}
	}

	init_timer();

	fd = open(PCIE215, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Err, can't open [%s].\n", PCIE215);
		fprintf(stderr, "Is the driver running?\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * Use pcie215_ioctl program to enable IRQ generation and configure of enabled pins.
	 *	./pcie215_ioctl -i 1 -a your_bit_mask
	 *	./pcie215_ioctl -i 0 -a 1
	 *
	 * In your program you would need to issue two ioctl calls.
	 *
	 * To enable interrupt generation programmatically
	 *		res = ioctl(fd, PCIE215_IOCTL_IRQ_ENABLE, 1);
	 *
	 * To specify enabled pins programmatically (i.e. to configure the PPI)
	 *		res = ioctl(fd, PCIE215_IOCTL_IRQ_TRIGGERS_ENABLE, your_bit_mask);
	 *
	 * If you don't do this and no other program do, then following read() system
	 * call will block until signal is delivered or driver gets finally configured
	 * and IRQ is asserted on enabled pin(s).
	 */
	res = read(fd, NULL, 0);
	if (res) {
		if (errno == EINTR)
			fprintf(stderr, "Err, interrupted by signal, read [%d] errno [%d][%s].\n",
				res, errno, strerror(errno));
		else
			fprintf(stderr, "Err, read failed [%d] errno [%d][%s].\n",
				res, errno, strerror(errno));

		exit(EXIT_FAILURE);
	}

	fprintf(stderr, "OK, read [%d].\n", res);

	return EXIT_SUCCESS;
}
