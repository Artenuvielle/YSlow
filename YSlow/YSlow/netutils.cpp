#include <iostream>

#include <stdlib.h>
#include <fcntl.h>

#include "netutils.h"

using namespace std;

void make_socket_non_blocking(int socket_fd)
{
	int flags;

	flags = fcntl(socket_fd, F_GETFL, 0);
	if (flags == -1) {
		cerr << "Couldn't get socket flags";
		exit(1);
	}

	flags |= O_NONBLOCK;
	if (fcntl(socket_fd, F_SETFL, flags) == -1) {
		cerr << "Couldn't set socket flags";
		exit(-1);
	}
}