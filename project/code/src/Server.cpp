#include "Server.hpp"

Server::Server() : _listenFd(-1), _running(false), _logger() {}

Server::Server(const Server &other) {
	(void)other;
	return;
}

Server& Server::operator=(const Server &other) {
	(void)other;
	return *this;
}

Server::~Server() { 
	this->stop();
}

bool Server::setNonBlocking(int fd) {
	int flags = ::fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return false;
	return (::fcntl(fd, F_SETFL, flags | O_NONBLOCK) == 0);
}

bool Server::applySockOpt(int fd) {
	if (SRV_SO_REUSEADDR) {
		int v = 1;
		if (::setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) != 0)
		return false;
	}
	if (SRV_SO_REUSEPORT) {
		int v = 1;
		if (::setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &v, sizeof(v)) != 0)
			return false;
	}
	if (SRV_TCP_KEEPALIVE) {
		int v = 1;
		if (::setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &v, sizeof(v)) != 0)
			return false;
	}
	return true;
}

bool Server::setupSocket() {
	_listenFd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (_listenFd < 0)
		return false;

	if (!this->applySockOpt(_listenFd)) {
		::close(_listenFd);
		_listenFd = -1;
		return false;
	}

	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SRV_PORT);

	in_addr ina;
	if (::inet_pton(AF_INET, SRV_ADDR, &ina) != 1) {
		::close(_listenFd);
		_listenFd = -1;
		return false;
	}
	addr.sin_addr = ina;

	if (::bind(_listenFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
		::close(_listenFd);
		_listenFd = -1;
		return false;
	}
	if (!this->setNonBlocking(_listenFd)) {
		::close(_listenFd);
		_listenFd = -1;
		return false;
	}
	if (::listen(_listenFd, SRV_MAX_CLIENTS) != 0) {
		::close(_listenFd);
		_listenFd = -1;
		return false;
	}

	_logger.info("Server listening on: " + std::string(SRV_ADDR) + ":" + std::to_string(SRV_PORT));
	return true;
}

bool Server::start() {
	if (_running)
		return true;
	if (!this->setupSocket()) {
		_logger.error("Failed to set up server socket: " + std::string(std::strerror(errno)));
		return false;
	}
	_running = true;
	return true;
}

void Server::stop() {
	if (!_running) return;
	_running = false;
	this->closeAll();
	if (_listenFd >= 0) {
		::close(_listenFd);
		_listenFd = -1;
	}
	_logger.info("Server stopped.");
	return;
}

bool Server::isRunning() const {
	return _running;
}

int Server::fd() const {
	return _listenFd;
}

int Server::clientCount() const {
	return static_cast<int>(_clients.size());
}

// patch: 'linger' pour forcer la fermeture immédiate (unit test failed 04.11)
void Server::closeAll() {
	for (std::unordered_map<int, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
		int fd = it->first;
		if (fd == _listenFd)
			continue;
		linger lg;
		lg.l_onoff = 1;
		lg.l_linger = 0;
		::setsockopt(fd, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));
		::shutdown(fd, SHUT_RDWR);
		::close(fd);
	}
	_clients.clear();
	_logger.info("All clients disconnected.");
}

void Server::acceptNew() {
	for (;;) {
		sockaddr_in cli;
		socklen_t len = sizeof(cli);
		int cfd = ::accept(_listenFd, reinterpret_cast<sockaddr*>(&cli), &len);
		if (cfd < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) break;
			_logger.error("accept failed: " + std::string(std::strerror(errno)));
			break;
		}

		if ((int)_clients.size() >= SRV_MAX_CLIENTS) {
			linger lg; lg.l_onoff = 1; lg.l_linger = 0;
			::setsockopt(cfd, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));

			const char *msg = "Error: too many clients connected, try again later.\n";
			(void)::send(cfd, msg, std::strlen(msg), 0);

			_logger.warn(std::string("Rejected connection from ")
				+ std::string(::inet_ntoa(cli.sin_addr)) + ":"
				+ std::to_string(ntohs(cli.sin_port))
				+ " - max clients reached.");

			::close(cfd);
			continue;
		}

		if (!setNonBlocking(cfd)) {
			::close(cfd);
			continue;
		}

		char ipStr[INET_ADDRSTRLEN] = {0};
		::inet_ntop(AF_INET, &(cli.sin_addr), ipStr, INET_ADDRSTRLEN);

		Client c;
		c.fd = cfd;
		c.port = ntohs(cli.sin_port);
		c.peer = std::string(ipStr);
		_clients[cfd] = c;

		_logger.info("client connected [" + std::to_string(c.fd) + "] " + c.peer + ":" + std::to_string(c.port));
	}
}

void Server::disconnect(int clientFd, const char *reason) {
    std::unordered_map<int, Client>::iterator it = _clients.find(clientFd);
    if (it == _clients.end()) return;

    if (reason)
        _logger.info("disconnecting client " + it->second.peer + ":" + std::to_string(it->second.port) + " - " + reason);
    else
        _logger.info("disconnecting client " + it->second.peer + ":" + std::to_string(it->second.port));

    linger lg;
    lg.l_onoff = 1;
    lg.l_linger = 0;
    ::setsockopt(clientFd, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));
    ::shutdown(clientFd, SHUT_RDWR);
    ::close(clientFd);
    _clients.erase(it);
}

bool Server::sendLine(int clientFd, const std::string &line) {
	std::unordered_map<int, Client>::iterator it = _clients.find(clientFd);
	if (it == _clients.end())
		return false;
	it->second.outputBuffer.append(line + "\n");
	return true;
}

// si on veux ajouter dautres commandes, le faire ici
bool Server::processLine(int clientFd, const std::string &line) {
	std::string trimmed = line;
	trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
	trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);

	if (trimmed == SRV_CMD_QUIT) {
		_logger.info("Received QUIT command from user [" + std::to_string(clientFd) + "].");
		this->stop();
		return false;
	}

	_logger.log("User [" + std::to_string(clientFd) + "] input: " + line);
	return true;
}

void Server::handleRead(int clientFd) {
	if (clientFd == _listenFd) {
		this->acceptNew();
		return;
	}
	std::unordered_map<int, Client>::iterator it = _clients.find(clientFd);
	if (it == _clients.end())
		return;

	char buffer[SRV_BUFFER_SIZE];
	for (;;) {
		ssize_t bytesRead = ::recv(clientFd, buffer, sizeof(buffer), 0);
		if (bytesRead > 0) {
			it->second.inputBuffer.append(buffer, static_cast<std::size_t>(bytesRead));
			for (;;) {
				std::size_t pos = it->second.inputBuffer.find('\n');
				if (pos == std::string::npos)
					break;
				std::string line = it->second.inputBuffer.substr(0, pos);
				if (!line.empty() && line.back() == '\r')
					line.pop_back();
				it->second.inputBuffer.erase(0, pos + 1);

				if (!this->processLine(clientFd, line))
					return;
			}
		} else if (bytesRead == 0) {
			this->disconnect(clientFd, "client closed connection");
			return;
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			this->disconnect(clientFd, "recv error");
			return;
		}
	}
}

void Server::handleWrite(int clientFd) {
	std::unordered_map<int, Client>::iterator it = _clients.find(clientFd);
	if (it == _clients.end())
		return;
	while (!it->second.outputBuffer.empty()) {
		ssize_t bytesSent = ::send(clientFd, it->second.outputBuffer.c_str(), it->second.outputBuffer.size(), 0);
		if (bytesSent > 0) {
			it->second.outputBuffer.erase(0, static_cast<std::size_t>(bytesSent));
		} else {
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			this->disconnect(clientFd, "send error");
			return;
		}
	}
}

void Server::tick() {
	int signal = md_signal_stop_requested();
	if (signal != 0) {
		_logger.info("Signal [" + std::to_string(signal) + "] \"" + (strsignal(signal) ? strsignal(signal) : "unknown") + "\" received, stopping daemon...");
		md_signal_clear();
		this->stop();
	}
}

void Server::run() {
	if (!_running)
		return;

	while (_running) {
		this->acceptNew();

		fd_set readFds, writeFds;
		FD_ZERO(&readFds);
		FD_ZERO(&writeFds);

		int maxFd = _listenFd;
		FD_SET(_listenFd, &readFds);

		for (std::unordered_map<int, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it) {
			FD_SET(it->first, &readFds);
			if (!it->second.outputBuffer.empty())
				FD_SET(it->first, &writeFds);
			if (it->first > maxFd)
				maxFd = it->first;
		}

		timeval timeout;
		timeout.tv_sec  = 0;
		timeout.tv_usec = SRV_TIMEOUT_USEC;

		int activity = ::select(maxFd + 1, &readFds, &writeFds, nullptr, &timeout);
		if (activity < 0) {
			if (errno == EINTR)
				continue;
			_logger.error("select error");
			break;
		}

		this->acceptNew();

		if (activity == 0) {
			this->tick();
			continue;
		}

		if (FD_ISSET(_listenFd, &readFds))
			this->handleRead(_listenFd);

		std::vector<int> fds;
		fds.reserve(_clients.size());
		for (std::unordered_map<int, Client>::const_iterator it = _clients.begin(); it != _clients.end(); ++it)
			fds.push_back(it->first);

		for (size_t i = 0; i < fds.size(); ++i) {
			int cliFd = fds[i];
			if (_clients.find(cliFd) == _clients.end())
				continue;
			if (FD_ISSET(cliFd, &readFds))
				handleRead(cliFd);
			if (_clients.find(cliFd) == _clients.end())
				continue;
			if (FD_ISSET(cliFd, &writeFds))
				handleWrite(cliFd);
		}
	}
}

Tintin_reporter &Server::getLogger() {
	return _logger;
}
