#pragma once

extern void handle_backend_socket_event(epoll_event_handler* self, uint32_t events);

extern void close_backend_socket(epoll_event_handler*);

extern epoll_event_handler* create_backend_socket_handler(int backend_socket_fd, epoll_event_handler* client_handler);