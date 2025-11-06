#include "MattDaemon.hpp"

static volatile sig_atomic_t s_md_stop_flag = 0;

int md_signal_stop_requested() {
	return s_md_stop_flag;
}

void md_signal_clear() {
	s_md_stop_flag = 0;
}

void md_signal_request_stop(int signum) {
	s_md_stop_flag = signum;
}