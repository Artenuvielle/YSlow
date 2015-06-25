#include <iostream>

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>

#include "netutils.h"
#include "epollinterface.h"
#include "backend_socket.h"
#include "client_socket.h"

using namespace std;

#define BUFFER_SIZE 4096

struct backend_socket_event_data {
	epoll_event_handler* client_handler;
};

void handle_backend_socket_event(epoll_event_handler* self, uint32_t events) {
	backend_socket_event_data* closure = (backend_socket_event_data*) self->closure;

	char buffer[BUFFER_SIZE];
	int bytes_read;

	if (events & EPOLLIN) {
		while ((bytes_read = read(self->fd, buffer, BUFFER_SIZE)) != -1 && bytes_read != 0) {
			if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
				return;
			}

			if (bytes_read == 0 || bytes_read == -1) {
				close_client_socket(closure->client_handler);
				close_backend_socket(self);
				return;
			}

			write_to_client(closure->client_handler, buffer, bytes_read);
		}
	}

	if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
		close_client_socket(closure->client_handler);
		close_backend_socket(self);
		return;
	}

}



void close_backend_socket(epoll_event_handler* self) {
	close(self->fd);
	free(self->closure);
	delete self;
}



struct epoll_event_handler* create_backend_socket_handler(int backend_socket_fd, epoll_event_handler* client_handler) {
	make_socket_non_blocking(backend_socket_fd);

	backend_socket_event_data* closure = new backend_socket_event_data;
	closure->client_handler = client_handler;

	epoll_event_handler* result = new epoll_event_handler;
	result->fd = backend_socket_fd;
	result->handle = handle_backend_socket_event;
	result->closure = closure;

	return result;
}
