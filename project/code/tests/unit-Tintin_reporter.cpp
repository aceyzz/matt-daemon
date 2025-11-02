#include "MattDaemon.hpp"
#include "Tintin_reporter.hpp"

static std::string join_path(const char* dir, const char* file) {
	std::string d(dir ? dir : "");
	if (!d.empty() && d.back() != '/') d.push_back('/');
	d.append(file ? file : "");
	return d;
}

static bool read_file_all(const std::string& path, std::string& out) {
	int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
	if (fd < 0) return false;
	out.clear();
	char buf[4096];
	for (;;) {
		ssize_t n = ::read(fd, buf, sizeof(buf));
		if (n < 0) { if (errno == EINTR) continue; ::close(fd); return false; }
		if (n == 0) break;
		out.append(buf, static_cast<size_t>(n));
	}
	::close(fd);
	return true;
}

int test_TintinReporter() {
	std::cout << CYAN "================ Tintin_reporter =============" RST "\n";

	Runner R;

	const std::string logDir  = MD_LOG_DIR;
	const std::string logFile = join_path(MD_LOG_DIR, MD_LOG_FILE);

	FileOps::ensureDir(MD_LOG_DIR, MD_LOG_DIR_MODE);
	{
		std::vector<std::string> entries;
		if (FileOps::listFiles(MD_LOG_DIR, entries)) {
			const std::string prefix = std::string(MD_LOG_FILE) + ".";
			for (size_t i = 0; i < entries.size(); ++i) {
				const std::string& n = entries[i];
				if (n == MD_LOG_FILE || n.rfind(prefix, 0) == 0) {
					FileOps::removeFile(join_path(MD_LOG_DIR, n.c_str()));
				}
			}
		}
	}

	Tintin_reporter tr;
	R.expectTrue("log directory exists", FileOps::exists(MD_LOG_DIR));
	R.expectTrue("log file exists after construction", FileOps::exists(logFile));

	R.expectTrue("info('hello-info')",  tr.info("hello-info"));
	R.expectTrue("warn('hello-warn')",  tr.warn("hello-warn"));
	R.expectTrue("error('hello-error')", tr.error("hello-error"));
	R.expectTrue("log('hello-log')",    tr.log("hello-log"));

	std::string content;
	R.expectTrue("read log file after writes", read_file_all(logFile, content));
	R.expectContains("contains [INFO]\\thello-info",  content, "] [INFO]\thello-info");
	R.expectContains("contains [WARN]\\thello-warn",  content, "] [WARN]\thello-warn");
	R.expectContains("contains [ERROR]\\thello-error",content, "] [ERROR]\thello-error");
	R.expectContains("contains [LOG]\\thello-log",    content, "] [LOG]\thello-log");

	uint64_t sz = 0; FileOps::sizeOf(logFile, sz);
	const uint64_t maxSz = static_cast<uint64_t>(MD_LOG_MAX_SIZE);
	const uint64_t need = (sz < maxSz) ? (maxSz - sz + 1024) : 2048;
	std::string big(need, 'X');
	R.expectTrue("write big line to trigger rotation", tr.info(std::string("rotate-test ") + big));

	std::vector<std::string> entries;
	R.expectTrue("list log dir after rotation attempt", FileOps::listFiles(MD_LOG_DIR, entries));
	size_t backupCount = 0;
	{
		const std::string prefix = std::string(MD_LOG_FILE) + ".";
		for (size_t i = 0; i < entries.size(); ++i)
			if (entries[i].rfind(prefix, 0) == 0) ++backupCount;
	}
	R.expectTrue("at least one rotated backup exists", backupCount >= 1);

	uint64_t newsz = 0;
	R.expectTrue("current log file size readable after rotation", FileOps::sizeOf(logFile, newsz));
	if (newsz) {
		R.expectTrue("current log size <= MD_LOG_MAX_SIZE", newsz <= maxSz);
	}

	std::cout << GRY1 "[CLEANUP] keeping logs in " << MD_LOG_DIR << RST "\n";
	std::cout << CYAN "==============================================" RST "\n";
	final_result(R.failingList, R.failures, R.assertions);
	std::cout << CYAN "==============================================" RST "\n";
	return R.failures;
}
