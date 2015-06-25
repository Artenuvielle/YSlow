#include <iostream>

#include <stdlib.h>
#include <signal.h>
#include <sys/epoll.h>
#include <lua.hpp>

#include "epollinterface.h"
#include "server_socket.h"
#include "Logger.h"

using namespace std;

char* get_config_opt(lua_State* L, char* name) {
	lua_getglobal(L, name);
	if (!lua_isstring(L, -1)) {
		cerr << name << " must be a string" << name;
		exit(1);
	}
	return (char*)lua_tostring(L, -1);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		cerr << "Usage: " << argv[0] << " <config_file>" << endl;
		exit(1);
	}
	cout << "---------------------------------------------" << endl;
	cout << "------------ YSlow Reverse Proxy ------------" << endl;
	cout << "---------------------------------------------" << endl << endl;

	lua_State *L = lua_open();

	if (luaL_dofile(L, argv[1]) != 0) {
		Logger::error << "Error parsing config file: " << lua_tostring(L, -1);
		exit(1);
	}
	char* server_port_str = get_config_opt(L, "listenPort");
	char* backend_addr = get_config_opt(L, "backendAddress");
	char* backend_port_str = get_config_opt(L, "backendPort");

	signal(SIGPIPE, SIG_IGN);

	int epoll_fd = epoll_create1(0);
	if (epoll_fd == -1) {
		Logger::error << "Couldn't create epoll FD" << endl;
		exit(1);
	}

	epoll_event_handler* server_socket_event_handler;
	server_socket_event_handler = create_server_socket_handler(epoll_fd, server_port_str, backend_addr, backend_port_str);
	add_epoll_handler(epoll_fd, server_socket_event_handler, EPOLLIN);

	Logger::info << "Started server. Listening on port " << server_port_str << "." << endl;
	do_reactor_loop(epoll_fd);

	return 0;
}
