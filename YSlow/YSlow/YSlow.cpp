#include <iostream>

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

using namespace std;

#define MAX_LISTEN_BACKLOG 1
#define BUFFER_SIZE 4096


void handle_client_connection(int client_socket_fd, char *backend_host, char *backend_port_str)
{
	addrinfo hints;
	addrinfo *addrs;
	addrinfo *addrs_iter;
	int getaddrinfo_error;

	int backend_socket_fd;

	char buffer[BUFFER_SIZE];
	int bytes_read;

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	getaddrinfo_error = getaddrinfo(backend_host, backend_port_str, &hints, &addrs);
	if (getaddrinfo_error != 0) {
		cerr << "Couldn't find backend: " << gai_strerror(getaddrinfo_error) << endl;
		exit(1);
	}

	for (addrs_iter = addrs; addrs_iter != NULL; addrs_iter = addrs_iter->ai_next) {
		backend_socket_fd = socket(addrs_iter->ai_family, addrs_iter->ai_socktype, addrs_iter->ai_protocol);
		if (backend_socket_fd == -1) {
			continue;
		}
		if (connect(backend_socket_fd, addrs_iter->ai_addr, addrs_iter->ai_addrlen) != -1) {
			break;
		}
		else {
			close(backend_socket_fd);
		}
	}
	if (addrs_iter == NULL) {
		cerr << "Couldn't connect to backend" << endl;
		exit(1);
	}
	freeaddrinfo(addrs);

	bytes_read = read(client_socket_fd, buffer, BUFFER_SIZE);
	cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl << buffer << endl;
	write(backend_socket_fd, buffer, bytes_read);

	cout << ">>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
	while (bytes_read = read(backend_socket_fd, buffer, BUFFER_SIZE)) {
		cout << buffer;
		write(client_socket_fd, buffer, bytes_read);
	}
	cout << endl;

	close(backend_socket_fd);
	close(client_socket_fd);
}

int main(int argc, char *argv[]) {
	char *server_port_str;
	char *backend_addr;
	char *backend_port_str;

	addrinfo hints;
	addrinfo *addrs;
	addrinfo *addr_iter;
	int getaddrinfo_error;

	int server_socket_fd;
	int client_socket_fd;

	int so_reuseaddr;

	if (argc != 4) {
		cerr << "Usage: " << argv[0] << " <server_port> <backend_addr> <backend_port>" << endl;
		exit(1);
	}

	cout << "---------------------------------------------" << endl;
	cout << "------------ YSlow Reverse Proxy ------------" << endl;
	cout << "---------------------------------------------" << endl;
	server_port_str = argv[1];
	backend_addr = argv[2];
	backend_port_str = argv[3];

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	getaddrinfo_error = getaddrinfo(NULL, server_port_str, &hints, &addrs);

	for (addr_iter = addrs; addr_iter != NULL; addr_iter = addr_iter->ai_next) {
		server_socket_fd = socket(addr_iter->ai_family, addr_iter->ai_socktype, addr_iter->ai_protocol);
		if (server_socket_fd == -1) {
			continue;
		}

		so_reuseaddr = 1;
		setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR, &so_reuseaddr, sizeof(so_reuseaddr));

		if (bind(server_socket_fd, addr_iter->ai_addr, addr_iter->ai_addrlen) == 0) {
			break;
		}

		close(server_socket_fd);
	}

	if (addr_iter == NULL) {
		cerr << "Couldn't bind" << endl;
		exit(1);
	}

	freeaddrinfo(addrs);

	listen(server_socket_fd, MAX_LISTEN_BACKLOG);

	while (1) {
		client_socket_fd = accept(server_socket_fd, NULL, NULL);
		if (client_socket_fd == -1) {
			cerr << "Could not accept" << endl;
			exit(1);
		}
		handle_client_connection(client_socket_fd, backend_addr, backend_port_str);
	}
}