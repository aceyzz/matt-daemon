#pragma once

#include "MattDaemon.hpp"
#include "Tintin_reporter.hpp"

class Server {
	public:
		Server();
		~Server();

		bool	start();
		void	stop();
		bool	isRunning() const;
		int		fd() const;
		int		clientCount() const;
		void	run();
		void	tick();

	private:
		// Desactiver les autres, pas besoin mais présent (Coplien)
		Server(const Server &other);
		Server &operator=(const Server &other);

		struct Client {
			int			fd;
			std::string	inputBuffer;
			std::string	outputBuffer;
			std::string	peer;
			uint16_t	port;
		};

		bool	setupSocket();
		void	closeAll();
		void	acceptNew();
		void	handleRead(int clientFd);
		void	handleWrite(int clientFd);
		void	disconnect(int clientFd, const char *reason = nullptr);
		bool	sendLine(int clientFd, const std::string &line);
		bool	processLine(int clientFd, const std::string &line);
		bool	setNonBlocking(int fd);
		bool	applySockOpt(int fd);

	private:
		int								_listenFd;
		bool							_running;
		Tintin_reporter					_logger;
		std::unordered_map<int, Client>	_clients;
};
