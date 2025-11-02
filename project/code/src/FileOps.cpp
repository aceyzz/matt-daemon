#include "FileOps.hpp"

FileOps::FileOps() : _fd(-1), _path() {}

FileOps::FileOps(const FileOps& other) : _fd(-1), _path() {
	if (!other._path.empty())
		this->bind(other._path, true);
}

FileOps& FileOps::operator=(const FileOps& other) {
	if (this == &other)
		return *this;
	this->unbind();
	if (!other._path.empty()) this->bind(other._path, true);
	return (*this);
}

FileOps::~FileOps() {
	this->unbind();
}

bool FileOps::bind(const std::string& path, bool appendMode) {
	this->unbind();
	_path = path;
	return (appendMode ? openAppendInternal(path, 0640) : openRWCreateInternal(path, 0640));
}

void FileOps::unbind() {
	if (_fd >= 0)
	{
		::close(_fd);
		_fd = -1;
	}
	_path.clear();
}

bool FileOps::write(const char* buf, size_t len) {
	const char*	ptr = buf;
	size_t		remaining = len;
	
	if (_fd < 0)
		return (false);
	while (remaining > 0)
	{
		ssize_t written = ::write(_fd, ptr, remaining);
		if (written < 0)
		{
			if (errno == EINTR) continue;
			return (false);
		}
		ptr += static_cast<size_t>(written);
		remaining -= static_cast<size_t>(written);
	}
	return (true);
}

bool FileOps::write(const std::string& s) {
	return (this->write(s.data(), s.size()));
}

bool FileOps::writeLine(const std::string& line) {
	if (!this->write(line))
		return (false);
	return (this->write("\n", 1));
}

bool FileOps::readAll(std::string& out) const {
	if (_path.empty())
		return (false);

	int fd = ::open(_path.c_str(), O_RDONLY | O_CLOEXEC);
	if (fd < 0)
		return (false);

	out.clear();
	char buf[4096];
	for (;;) {
		ssize_t bytesRead = ::read(fd, buf, sizeof(buf));
		if (bytesRead < 0) { if (errno == EINTR) continue; ::close(fd); return (false); }
		if (bytesRead == 0) break;
		out.append(buf, static_cast<size_t>(bytesRead));
	}
	::close(fd);
	return (true);
}

bool FileOps::flush() {
	if (_fd < 0)
		return (false);
	return (::fsync(_fd) == 0);
}

bool FileOps::reopenAppend() {
	if (_path.empty())
		return (false);
	return (this->reopenAppendInternal());
}

bool FileOps::ensureDir(const std::string& dirPath, mode_t mode) {
	if (dirPath.empty()) return (false);
	if (exists(dirPath)) return (true);

	std::string			acc = (dirPath[0] == '/') ? "/" : "";
	std::stringstream	ss(dirPath);
	std::string			part;
	while (std::getline(ss, part, '/')) {
		if (part.empty())
			continue;
		if (!acc.empty() && acc.back() != '/')
			acc += '/';
		acc += part;
		if (::mkdir(acc.c_str(), mode) == -1)
		{
			if (errno == EEXIST)
				continue;
			if (!exists(acc))
				return (false);
		}
	}
	return (true);
}

bool FileOps::exists(const std::string& path) {
	return (::access(path.c_str(), F_OK) == 0);
}

bool FileOps::sizeOf(const std::string& path, uint64_t& outBytes) {
	struct stat	st;
	if (::stat(path.c_str(), &st) != 0)
		return (false);
	outBytes = static_cast<uint64_t>(st.st_size);
	return (true);
}

bool FileOps::removeFile(const std::string& path) {
	return (::unlink(path.c_str()) == 0);
}

bool FileOps::renameFile(const std::string& from, const std::string& to) {
	return (::rename(from.c_str(), to.c_str()) == 0);
}

bool FileOps::listFiles(const std::string& dirPath, std::vector<std::string>& out) {
	out.clear();
	DIR*	dir = ::opendir(dirPath.c_str());
	if (!dir)
		return (false);

	struct dirent*	entry;
	while ((entry = ::readdir(dir)) != nullptr)
	{
		std::string	name(entry->d_name);
		if (name != "." && name != "..")
			out.push_back(name);
	}
	::closedir(dir);
	std::sort(out.begin(), out.end());
	return (true);
}

int FileOps::openFd(const std::string& path, int flags, mode_t mode) {
	return (::open(path.c_str(), flags | O_CLOEXEC, mode));
}

void FileOps::closeFd(int fd) {
	if (fd >= 0)
		::close(fd);
}

bool FileOps::lockExclusiveNonBlock(int fd) {
	if (fd < 0)
		return (false);
	return (::flock(fd, LOCK_EX | LOCK_NB) == 0);
}

// utile pour env
static inline std::string md_ltrim(std::string s) {
	size_t i = 0;
	while (i < s.size() && (s[i] == ' ' || s[i] == '\t')) ++i;
	return s.substr(i);
}
static inline std::string md_rtrim(std::string s) {
	while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.pop_back();
	return s;
}
static inline std::string md_trim(std::string s) { return md_rtrim(md_ltrim(s)); }

bool FileOps::readKeyValuesFile(const std::string& path, std::unordered_map<std::string,std::string>& kv) {
	std::ifstream in(path.c_str());
	if (!in) return false;
	std::string line;
	while (std::getline(in, line)) {
		if (line.empty() || line[0] == '#') continue;
		std::string::size_type p = line.find('=');
		if (p == std::string::npos) continue;
		std::string k = md_trim(line.substr(0, p));
		std::string v = md_trim(line.substr(p + 1));
		if (!k.empty()) kv[k] = v;
	}
	return true;
}

bool FileOps::openAppendInternal(const std::string& path, mode_t mode) {
	_fd = ::open(path.c_str(), O_WRONLY | O_APPEND | O_CREAT | O_CLOEXEC, mode);
	return (_fd >= 0);
}

bool FileOps::openRWCreateInternal(const std::string& path, mode_t mode) {
	_fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_CLOEXEC, mode);
	return (_fd >= 0);
}

bool FileOps::reopenAppendInternal() {
	if (_path.empty()) return (false);
	int old = _fd;
	_fd = ::open(_path.c_str(), O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0640);
	if (_fd < 0)
	{
		_fd = old;
		return (false);
	}
	if (old >= 0)
		::close(old);
	return (true);
}
