#pragma once

#include "MattDaemon.hpp"

class Tintin_reporter {
	public:
		Tintin_reporter();
		Tintin_reporter(const Tintin_reporter& other);
		Tintin_reporter& operator=(const Tintin_reporter& other);
		~Tintin_reporter();

		bool log(const std::string& msg);
		bool info(const std::string& msg);
		bool warn(const std::string& msg);
		bool error(const std::string& msg);

	private:
		enum Level { LOG, INFO, WARN, ERROR };

		bool	write(Level level, const std::string& msg);
		static const char*	levelToStr(Level level);
		static std::string	getTimestampNow();
		bool	ensureReady();
		bool	rotateIfNeeded(std::size_t incomingBytes = 0);
		std::string	formatLine(Level level, const std::string& msg);

	private:
		FileOps		_fops;
		bool		_ready;
		std::size_t	_actualSize;
};
