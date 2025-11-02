#include "MattDaemon.hpp"
#include "Server.hpp"
#include <signal.h>

static Server* g_srv = 0;

static void sigterm_handler(int) {
	if (g_srv) g_srv->stop();
}

static bool write_all(int fd, const char* buf, size_t len) {
	const char* p = buf;
	size_t r = len;
	for (;;) {
		if (r == 0) return true;
		ssize_t n = ::send(fd, p, r, MSG_NOSIGNAL);
		if (n > 0) { p += (size_t)n; r -= (size_t)n; continue; }
		if (n < 0 && errno == EINTR) continue;
		return false;
	}
}

static int make_client() {
	int fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return -1;
	sockaddr_in a; std::memset(&a, 0, sizeof(a));
	a.sin_family = AF_INET;
	a.sin_port = htons(SRV_PORT);
	if (::inet_pton(AF_INET, "127.0.0.1", &a.sin_addr) != 1) { ::close(fd); return -1; }
	if (::connect(fd, (sockaddr*)&a, sizeof(a)) != 0) { ::close(fd); return -1; }
	return fd;
}

int test_Server() {
	std::cout << CYAN "====================== Server ======================" RST "\n";
	Runner R;
	::signal(SIGPIPE, SIG_IGN);

	int pipes[2] = {-1,-1};
	bool ok = (::pipe(pipes) == 0);
	R.expectTrue("pipe()", ok);
	if (!ok) { final_result(R.failingList, R.failures, R.assertions); return R.failures; }

	pid_t pid = ::fork();
	R.expectTrue("fork()", pid >= 0);

	if (pid == 0) {
		::signal(SIGTERM, sigterm_handler);
		::close(pipes[0]);
		{
			Server s;
			g_srv = &s;
			bool started = s.start();
			if (started) { char b='R'; ::write(pipes[1], &b, 1); }
			s.run();
			g_srv = 0;
		}
		::_exit(0);
	}

	::close(pipes[1]);
	char b = 0;
	ssize_t n = ::read(pipes[0], &b, 1);
	::close(pipes[0]);
	R.expectTrue("server started (sync)", n == 1 && b == 'R');

	int c1 = make_client();
	R.expectTrue("connect c1", c1 >= 0);

	bool sentQuit = write_all(c1, SRV_CMD_QUIT "\n", sizeof(SRV_CMD_QUIT) + 1);
	R.expectTrue("c1 send quit", sentQuit);

	::usleep(150000);
	bool stillOpen = write_all(c1, "x", 1);
	R.expectTrue("c1 closed by server after quit", !stillOpen);
	if (c1 >= 0) ::close(c1);

	::kill(pid, SIGTERM);
	int status = 0;
	pid_t wp = ::waitpid(pid, &status, 0);
	R.expectTrue("waitpid(server)", wp == pid);

	std::cout << CYAN "====================================================" RST "\n";
	final_result(R.failingList, R.failures, R.assertions);
	std::cout << CYAN "====================================================" RST "\n";
	return R.failures;
}