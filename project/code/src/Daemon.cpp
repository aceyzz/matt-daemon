#include "Daemon.hpp"
#include "Tintin_reporter.hpp"

Daemon* Daemon::_instance = nullptr;

Daemon::Daemon() : _lockFd(-1), _running(false), _server(), _fops() {}
Daemon::Daemon(const Daemon&) {}
Daemon& Daemon::operator=(const Daemon&) { return *this; }
Daemon::~Daemon() { stop(); }

void	Daemon::_ignoreSignal(int signum) {
	struct sigaction	sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	::sigaction(signum, &sa, nullptr);
}

void Daemon::_onSignal(int signum) {
	md_signal_request_stop(signum);
}

bool Daemon::daemonize() {
	// 1er fork
	pid_t pid = ::fork();
	if (pid < 0)
		return false;

	// kill du parent
	if (pid > 0)
		exit(0);

	// devenir session leader
	if (::setsid() < 0)
		return false;

	// 2eme fork
	pid = ::fork();
	if (pid < 0)
		return false;

	// kill du parent
	if (pid > 0)
		exit(0);

	::umask(027);
	if (::chdir("/") != 0)
		return false;

	// redirs des fd dans /dev/null
	int nullfd = ::open("/dev/null", O_RDWR);
	if (nullfd >= 0) {
		::dup2(nullfd, STDIN_FILENO);
		::dup2(nullfd, STDOUT_FILENO);
		::dup2(nullfd, STDERR_FILENO);
		if (nullfd > 2)
			::close(nullfd);
	}

	_ignoreSignal(SIGPIPE);
	_server.getLogger().info("Daemonized successfully");
	return true;
}

bool Daemon::writePid() {
	if (_lockFd < 0)
		return false;
	
	if (::ftruncate(_lockFd, 0) != 0)
		return false;

	// pid dans lockfile
	char buf[64];
	int len = std::snprintf(buf, sizeof(buf), "%ld\n", (long)::getpid());
	if (len <= 0)
		return false;
	ssize_t n = ::write(_lockFd, buf, (size_t)len);
	return (n == len);
}

bool Daemon::acquireLock() {
	_fops.ensureDir("/var/lock", 0755);
	_lockFd = _fops.openFd(MD_LOCK_FILE, O_RDWR | O_CREAT, MD_LOCK_FILE_MODE);
	if (_lockFd < 0)
		return false;
	if (!_fops.lockExclusiveNonBlock(_lockFd)) {
		_fops.closeFd(_lockFd);
		_lockFd = -1;
		return false;
	}
	return true;
}

void Daemon::releaseLock() {
	if (_lockFd >= 0) {
		::flock(_lockFd, LOCK_UN);
		_fops.closeFd(_lockFd);
		_lockFd = -1;
		_fops.removeFile(MD_LOCK_FILE);
		_server.getLogger().info("Lock released");
	}
}

int Daemon::start() {
	if (_running)
		return 0;

	if (!acquireLock()) {
		const std::string msg = std::string("Can't open :") + MD_LOCK_FILE + "\n";
		::write(STDERR_FILENO, msg.c_str(), msg.size());
		Tintin_reporter tr;
		tr.error(std::string("Error file locked: ") + MD_LOCK_FILE);
		return EXIT_FAILURE;
	}

	if (!daemonize()) {
		Tintin_reporter tr;
		tr.error("daemonize failed");
		releaseLock();
		return EXIT_FAILURE;
	}

	if (!writePid()) {
		Tintin_reporter tr;
		tr.error("writePid failed");
		releaseLock();
		return EXIT_FAILURE;
	}

	_instance = this;
	_running = true;
	_server.getLogger().info("Daemon started");

	struct sigaction sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &_onSignal;
	::sigaction(SIGINT,  &sa, 0);
	::sigaction(SIGTERM, &sa, 0);
	::sigaction(SIGHUP,  &sa, 0);
	::sigaction(SIGQUIT, &sa, 0);

	if (!_server.start()) {
		releaseLock();
		_running = false;
		_instance = nullptr;
		_server.getLogger().error("Server start failed");
		return EXIT_FAILURE;
	}

	_server.run();
	stop();
	return 0;
}

void Daemon::stop() {
	if (!_running)
		return;

	_server.stop();
	releaseLock();
	_running = false;
	_instance = nullptr;
	_server.getLogger().info("Daemon stopped");
}
