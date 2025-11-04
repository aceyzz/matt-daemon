#pragma once

#include "MattDaemon.hpp"
#include "Server.hpp"

class Daemon {
	public:
		Daemon();
		~Daemon();

		int		start();
		void	stop();

	private:
		Daemon(const Daemon&);
		Daemon& operator=(const Daemon&);

		static Daemon* _instance;
		static void		_onSignal(int signum);
		static void		_ignoreSignal(int signum);

		bool	daemonize();
		bool	acquireLock();
		void	releaseLock();
		bool	writePid();

	private:
		int			_lockFd;
		bool		_running;
		Server		_server;
		FileOps		_fops;
};
