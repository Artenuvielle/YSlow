#pragma once

class EPollEventHandler {
    public:
        EPollEventHandler(int file_descriptor);
        virtual void handleEPollEvent(uint32_t event_mask) = 0;
        int getFileDescriptor();
    private:
        int fd;
};

class EPollManager {
    public:
        EPollManager();
        ~EPollManager();
        void addEventHandler(EPollEventHandler* handler, uint32_t event_mask);
        void removeEventHandler(EPollEventHandler* handler);
        void freeMemoryOnNextCycle(void* block);
        void executePollingLoop();
    private:
        int epoll_fd;
        struct free_list_entry {
            void* block;
            free_list_entry* next;
        };
        free_list_entry* free_list = NULL;
        void ProcessFreeingList();
};