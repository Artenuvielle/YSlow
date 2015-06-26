#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>
#include <string.h>

#include "epollinterface.h"
#include "connection.h"
#include "Logger.h"
#include "netutils.h"

#define BUFFER_SIZE 4096

struct data_buffer_entry {
	int is_close_message;
	char* data;
	int current_offset;
	int len;
	data_buffer_entry* next;
};

void connection_really_close(struct epoll_event_handler* self) {
	connection_closure* closure = (connection_closure*) self->closure;
	data_buffer_entry* next;
	while (closure->write_buffer != NULL) {
		next = closure->write_buffer->next;
		if (!closure->write_buffer->is_close_message) {
			epoll_add_to_free_list(closure->write_buffer->data);
		}
		epoll_add_to_free_list(closure->write_buffer);
		closure->write_buffer = next;
	}

	epoll_remove_handler(self);
	close(self->fd);
	epoll_add_to_free_list(self->closure);
	epoll_add_to_free_list(self);
	Logger::debug << "Freed connection " << self << endl;
}


void connection_on_close_event(epoll_event_handler* self) {
	connection_closure* closure = (connection_closure*) self->closure;
	if (closure->on_close != NULL) {
		closure->on_close(closure->on_close_closure);
	}
	connection_close(self);
}


void connection_on_out_event(epoll_event_handler* self) {
	connection_closure* closure = (connection_closure*) self->closure;
	int written;
	int to_write;
	data_buffer_entry* temp;
	while (closure->write_buffer != NULL) {
		if (closure->write_buffer->is_close_message) {
			connection_really_close(self);
			return;
		}

		to_write = closure->write_buffer->len - closure->write_buffer->current_offset;
		written = write(self->fd, closure->write_buffer->data + closure->write_buffer->current_offset, to_write);
		if (written != to_write) {
			if (written == -1) {
				if (errno == ECONNRESET || errno == EPIPE) {
					Logger::error << "On out event write error" << endl;
					connection_on_close_event(self);
					return;
				}
				if (errno != EAGAIN && errno != EWOULDBLOCK) {
					Logger::error << "Error writing to client" << endl;
					exit(-1);
				}
				written = 0;
			}
			closure->write_buffer->current_offset += written;
			break;
		} else {
			temp = closure->write_buffer;
			closure->write_buffer = closure->write_buffer->next;
			epoll_add_to_free_list(temp->data);
			epoll_add_to_free_list(temp);
		}
	}
}


void connection_on_in_event(epoll_event_handler* self) {
	connection_closure* closure = (connection_closure*) self->closure;
	char read_buffer[BUFFER_SIZE];
	int bytes_read;

	while ((bytes_read = read(self->fd, read_buffer, BUFFER_SIZE)) != -1 && bytes_read != 0) {
		if (bytes_read == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			return;
		}

		if (bytes_read == 0 || bytes_read == -1) {
			connection_on_close_event(self);
			return;
		}

		if (closure->on_read != NULL) {
			closure->on_read(closure->on_read_closure, read_buffer, bytes_read);
		}
	}
}


void connection_handle_event(epoll_event_handler* self, uint32_t events) {
	if (events & EPOLLOUT) {
		connection_on_out_event(self);
	}

	if (events & EPOLLIN) {
		connection_on_in_event(self);
	}

	if ((events & EPOLLERR) | (events & EPOLLHUP) | (events & EPOLLRDHUP)) {
		connection_on_close_event(self);
	}

}


void add_write_buffer_entry(connection_closure* closure, data_buffer_entry* new_entry) {
	data_buffer_entry* last_buffer_entry;
	if (closure->write_buffer == NULL) {
		closure->write_buffer = new_entry;
	} else {
		for (last_buffer_entry = closure->write_buffer; last_buffer_entry->next != NULL; last_buffer_entry = last_buffer_entry->next)
			;
		last_buffer_entry->next = new_entry;
	}
}


void connection_write(epoll_event_handler* self, char* data, int len) {
	connection_closure* closure = (connection_closure*) self->closure;

	int written = 0;
	if (closure->write_buffer == NULL) {
		written = write(self->fd, data, len);
		if (written == len) {
			return;
		}
	}
	if (written == -1) {
		if (errno == ECONNRESET || errno == EPIPE) {
			Logger::error << "Connection write error" << endl;
			connection_on_close_event(self);
			return;
		}
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			Logger::error << "Error writing to client" << endl;
			exit(-1);
		}
		written = 0;
	}

	int unwritten = len - written;
	data_buffer_entry* new_entry = new data_buffer_entry;
	new_entry->is_close_message = 0;
	new_entry->data = new char[unwritten];
	memcpy(new_entry->data, data + written, unwritten);
	new_entry->current_offset = 0;
	new_entry->len = unwritten;
	new_entry->next = NULL;

	add_write_buffer_entry(closure, new_entry);
}


void connection_close(epoll_event_handler* self) {
	connection_closure* closure = (connection_closure*) self->closure;
	closure->on_read = NULL;
	closure->on_close = NULL;
	if (closure->write_buffer == NULL) {
		connection_really_close(self);
	} else {
		data_buffer_entry* new_entry = new data_buffer_entry;
		new_entry->is_close_message = 1;
		new_entry->next = NULL;

		add_write_buffer_entry(closure, new_entry);
	}
}


struct epoll_event_handler* create_connection(int client_socket_fd) {
	make_socket_non_blocking(client_socket_fd);

	connection_closure* closure = new connection_closure;
	closure->write_buffer = NULL;

	epoll_event_handler* result = new epoll_event_handler;
	Logger::debug << "Created connection epoll handler " << result << endl;
	result->fd = client_socket_fd;
	result->handle = connection_handle_event;
	result->closure = closure;


	epoll_add_handler(result, EPOLLIN | EPOLLRDHUP | EPOLLET | EPOLLOUT);

	return result;
}