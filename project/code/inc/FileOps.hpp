#pragma once

#include "MattDaemon.hpp"

class FileOps
{
	public:
		FileOps();
		FileOps(const FileOps& other);
		FileOps& operator=(const FileOps& other);
		~FileOps();

		bool bind(const std::string& path, bool appendMode = true);
		void unbind();

		bool write(const char* buf, size_t len);
		bool write(const std::string& s);
		bool writeLine(const std::string& line);
		bool readAll(std::string& out) const;
		bool flush();
		bool reopenAppend();

		static bool ensureDir(const std::string& dirPath, mode_t mode = 0750);
		static bool exists(const std::string& path);
		static bool sizeOf(const std::string& path, uint64_t& outBytes);
		static bool removeFile(const std::string& path);
		static bool renameFile(const std::string& from, const std::string& to);
		static bool listFiles(const std::string& dirPath, std::vector<std::string>& out);

		static int  openFd(const std::string& path, int flags, mode_t mode = 0644);
		static void closeFd(int fd);
		static bool lockExclusiveNonBlock(int fd);

		static bool readKeyValuesFile(const std::string& path, std::unordered_map<std::string,std::string>& kv);

	private:
		bool openAppendInternal(const std::string& path, mode_t mode = 0640);
		bool openRWCreateInternal(const std::string& path, mode_t mode = 0640);
		bool reopenAppendInternal();

	private:
		int         _fd;
		std::string _path;
};
