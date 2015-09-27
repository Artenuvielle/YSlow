#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>

#include "EPoll.h"
#include "../libyslow/Logger.h"

using namespace std;

EPollManager::EPollManager() {
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        Logger::error << "Couldn't create epoll FD" << endl;
        exit(1);
    }
}

EPollManager::~EPollManager() {
    ProcessFreeingList();
}

void EPollManager::addEventHandler(EPollEventHandler* handler, u_int32_t event_mask) {
    epoll_event event;

    memset(&event, 0, sizeof(epoll_event));
    event.data.ptr = handler;
    event.events = event_mask;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, handler->getFileDescriptor(), &event) == -1) {
        Logger::error << "Couldn't register server socket with epoll" << endl;
        exit(-1);
    }
}

void EPollManager::removeEventHandler(EPollEventHandler * handler) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, handler->getFileDescriptor(), NULL);
}

void EPollManager::freeMemoryOnNextCycle(void * block) {
    free_list_entry* entry = new free_list_entry;
    entry->block = block;
    entry->next = free_list;
    free_list = entry;
}

void EPollManager::executePollingLoop() {
    epoll_event current_epoll_event;

    while (1) {
        EPollEventHandler* handler;

        epoll_wait(epoll_fd, &current_epoll_event, 1, -1);
        handler = (EPollEventHandler*)current_epoll_event.data.ptr;
        u_int32_t event = current_epoll_event.events;
        handler->handleEPollEvent(event);

        ProcessFreeingList();
    }
}

void EPollManager::ProcessFreeingList() {
    free_list_entry* temp;
    while (free_list != NULL) {
        free(free_list->block);
        temp = free_list->next;
        free(free_list);
        free_list = temp;
    }
}

EPollEventHandler::EPollEventHandler(int file_descriptor) : fd(file_descriptor) {
}

int EPollEventHandler::getFileDescriptor() {
    return fd;
}