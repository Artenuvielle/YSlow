#pragma once

#include "iostream"

using namespace std;

enum LogLevel {
	LOG_LEVEL_ALL = 0,
	LOG_LEVEL_DEBUG = 0,
	LOG_LEVEL_INFO = 1,
	LOG_LEVEL_WARN = 2,
	LOG_LEVEL_ERROR = 3,
	LOG_LEVEL_OFF = 4
};

class Logger {
	private:
		static int logging_level;
		Logger();
	public:
		static ostream& debug;
		static ostream& info;
		static ostream& warn;
		static ostream& error;
		static void setLoggingLevel(int);
		static int getLoggingLevel();
};

