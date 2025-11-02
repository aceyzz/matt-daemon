#include "Tintin_reporter.hpp"

static std::string md_join_path(const char* dir, const char* file) {
	std::string d(dir ? dir : "");
	if (!d.empty() && d.back() != '/')
		d.push_back('/');
	d.append(file ? file : "");
	return d;
}

Tintin_reporter::Tintin_reporter() : _ready(false), _actualSize(0) {
	this->ensureReady();
}

Tintin_reporter::Tintin_reporter(const Tintin_reporter& other) : _ready(false), _actualSize(0) {
	this->ensureReady();
	(void)other;
}

Tintin_reporter& Tintin_reporter::operator=(const Tintin_reporter& other) {
	if (this != &other) {
		this->ensureReady();
	}
	(void)other;
	return *this;
}

Tintin_reporter::~Tintin_reporter() {}

bool Tintin_reporter::log(const std::string& msg)  { return write(LOG,  msg); }
bool Tintin_reporter::info(const std::string& msg) { return write(INFO, msg); }
bool Tintin_reporter::warn(const std::string& msg) { return write(WARN, msg); }
bool Tintin_reporter::error(const std::string& msg){ return write(ERROR,msg); }

const char* Tintin_reporter::levelToStr(Level level) {
	switch (level) {
		case LOG:   return "LOG";
		case INFO:  return "INF";
		case WARN:  return "WRN";
		case ERROR: return "ERR";
	}
	return "LOG";
}

std::string Tintin_reporter::getTimestampNow() {
	char buf[32];
	std::time_t t = std::time(NULL);
	std::tm lt;
	std::tm* p = ::localtime_r(&t, &lt);
	std::strftime(buf, sizeof(buf), "%d/%m/%Y-%H:%M:%S", p ? p : std::localtime(&t));
	return std::string(buf);
}

std::string Tintin_reporter::formatLine(Level level, const std::string& msg) {
	std::string s;
	s.reserve(32 + 8 + msg.size());
	s.push_back('[');
	s += getTimestampNow();
	s += "] [ ";
	s += levelToStr(level);
	s += " ] - MattDaemon: ";
	s += msg;
	return s;
}

bool Tintin_reporter::ensureReady() {
	if (_ready)
		return true;

	if (!FileOps::ensureDir(MD_LOG_DIR, MD_LOG_DIR_MODE))
		return false;

	std::string path = md_join_path(MD_LOG_DIR, MD_LOG_FILE);
	if (!_fops.bind(path, true))
		return false;

	uint64_t sz = 0;
	if (FileOps::sizeOf(path, sz))
		_actualSize = sz;
	else
		_actualSize = 0;

	_ready = true;
	return true;
}

bool Tintin_reporter::rotateIfNeeded(std::size_t incomingBytes) {
	if (!_ready) return false;
	if (MD_LOG_MAX_SIZE == 0) return true;
	
	uint64_t projected = _actualSize + static_cast<uint64_t>(incomingBytes);
	if (projected <= static_cast<uint64_t>(MD_LOG_MAX_SIZE))
		return true;

	std::string base = md_join_path(MD_LOG_DIR, MD_LOG_FILE);
	std::string backup = base + "." + std::to_string(std::time(NULL));

	FileOps::renameFile(base, backup);
	_fops.reopenAppend();

	uint64_t sz = 0;
	if (FileOps::sizeOf(base, sz))
		_actualSize = sz;
	else
		_actualSize = 0;

	if (MD_LOG_BACKUP_COUNT > 0) {
		std::vector<std::string> entries;
		if (FileOps::listFiles(MD_LOG_DIR, entries)) {
			std::vector<std::string> backups;
			std::string prefix = std::string(MD_LOG_FILE) + ".";
			for (size_t i = 0; i < entries.size(); ++i) {
				const std::string& n = entries[i];
				if (n.size() > prefix.size() && n.compare(0, prefix.size(), prefix) == 0)
					backups.push_back(n);
			}
			std::sort(backups.begin(), backups.end());
			if (backups.size() > static_cast<size_t>(MD_LOG_BACKUP_COUNT)) {
				size_t toDel = backups.size() - static_cast<size_t>(MD_LOG_BACKUP_COUNT);
				for (size_t i = 0; i < toDel; ++i) {
					std::string fullPath = md_join_path(MD_LOG_DIR, backups[i].c_str());
					FileOps::removeFile(fullPath);
				}
			}
		}
	}
	return true;
}

bool Tintin_reporter::write(Level level, const std::string& msg) {
	if (!ensureReady()) return false;

	std::string line = formatLine(level, msg);
	if (!rotateIfNeeded(line.size() + 1)) return false;

	bool ok = _fops.writeLine(line);
	if (ok) {
		_actualSize += static_cast<uint64_t>(line.size() + 1);
		_fops.flush();
		if (_actualSize > static_cast<uint64_t>(MD_LOG_MAX_SIZE))
			rotateIfNeeded(0);
	}
	return ok;
}
