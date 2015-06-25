#include <iostream>

#include <sys/epoll.h>
#include <stdlib.h>

#include "epollinterface.h"

using namespace std;

void add_epoll_handler(int epoll_fd, epoll_event_handler* handler, uint32_t event_mask)
{
	epoll_event event;

	event.data.ptr = handler;
	event.events = event_mask;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handler->fd, &event) == -1) {
		cerr << "Couldn't register server socket with epoll";
		exit(-1);
	}
}

void do_reactor_loop(int epoll_fd)
{
	epoll_event current_epoll_event;

	while (1) {
		epoll_event_handler* handler;

		epoll_wait(epoll_fd, &current_epoll_event, 1, -1);
		handler = (epoll_event_handler*) current_epoll_event.data.ptr;
		handler->handle(handler, current_epoll_event.events);
	}

}