#include "exchange_simulator.h"
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <algorithm>

ClientManager::ClientManager() {
    epoll_fd_ = epoll_create1(0);
}

void ClientManager::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void ClientManager::add_listener(int listen_fd) {
    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd, &ev);
}

void ClientManager::handle_new_connection(int listen_fd) {
    while (true) {
        int fd = accept(listen_fd, nullptr, nullptr);
        if (fd < 0) {
            if (errno == EAGAIN) break;
            return;
        }
        set_nonblocking(fd);

        epoll_event ev{};
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = fd;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);

        clients_.push_back(fd);
        std::cout << "Client connected: fd=" << fd << "\n";
    }
}

void ClientManager::disconnect(int fd) {
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
    close(fd);
    clients_.erase(std::remove(clients_.begin(), clients_.end(), fd), clients_.end());
}

void ClientManager::broadcast(const void* data, size_t len) {
    for (int fd : clients_) {
        ssize_t n = send(fd, data, len, MSG_DONTWAIT);
        if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            disconnect(fd);
        }
    }
}

void ClientManager::handle_events(int listen_fd) {
    // epoll_event events[128];
    // int n = epoll_wait(epoll_fd_, events, 128, 0);

    // for (int i = 0; i < n; ++i) {
    //     if (events[i].events & (EPOLLHUP | EPOLLERR)) {
    //         disconnect(events[i].data.fd);
    //     }
    // }

    epoll_event events[128];
    int n = epoll_wait(epoll_fd_, events, 128, 0);

    for (int i = 0; i < n; ++i) {
        int fd = events[i].data.fd;

        if (fd == listen_fd) {
            handle_new_connection(listen_fd);
        } else if (events[i].events & (EPOLLHUP | EPOLLERR)) {
            disconnect(fd);
        }
    }
}
