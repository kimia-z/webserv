#include <ServerMain.hpp>

void ServerMain::setUpEpoll()
{
	epollFd_ = epoll_create1(0);
	if (epollFd_ == -1){
		std::cerr << "epoll_create1() failed: " << strerror(errno) << std::endl;
		//throw std::runtime_error("Failed to create epoll instance");
	}
	std::cout << "Epoll instance created (FD: " << epollFd_ << ")" << std::endl;
	addFdToEpoll(serverFd_, EPOLLIN);
}

void ServerMain::addFdToEpoll(int fd, uint32_t events)
{
	epoll_event event;
	event.events = events;
	event.data.fd = fd;
	if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, fd, &event) == -1){
		std::cerr << "epoll_ctl() failed to add fd: " << strerror(errno) << std::endl;
		//throw std::runtime_error("Failed to add fd to epoll");
	}
	std::cout << "Added FD " << fd << " to epoll with events " << events << std::endl;
}