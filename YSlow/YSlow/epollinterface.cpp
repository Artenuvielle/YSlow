#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

#include "epollinterface.h"
#include "Logger.h"


int epoll_fd;


void epoll_init() {
	epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		Logger::error << "Couldn't create epoll FD" << endl;
		exit(1);
	}
}


void epoll_add_handler(epoll_event_handler* handler, uint32_t event_mask) {
	epoll_event event;

	memset(&event, 0, sizeof(epoll_event));
	event.data.ptr = handler;
	event.events = event_mask;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handler->fd, &event) == -1) {
		Logger::error << "Couldn't register server socket with epoll" << endl;
		exit(-1);
	}
}


void epoll_remove_handler(epoll_event_handler* handler) {
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, handler->fd, NULL);
}


struct free_list_entry {
	void* block;
	free_list_entry* next;
};

free_list_entry* free_list = NULL;


void epoll_add_to_free_list(void* block) {
	free_list_entry* entry = new free_list_entry;
	entry->block = block;
	entry->next = free_list;
	free_list = entry;
}


void epoll_do_reactor_loop() {
	epoll_event current_epoll_event;

	while (1) {
		epoll_event_handler* handler;

		epoll_wait(epoll_fd, &current_epoll_event, 1, -1);
		handler = (epoll_event_handler*) current_epoll_event.data.ptr;
		handler->handle(handler, current_epoll_event.events);

		free_list_entry* temp;
		while (free_list != NULL) {
			free(free_list->block);
			temp = free_list->next;
			free(free_list);
			free_list = temp;
		}
	}
}