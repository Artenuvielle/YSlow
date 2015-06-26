#include "Logger.h"

#include <sstream>

struct LoggingLevelData {
	LogLevel level;
	string name;
	ostream& stream;
};

class LoggingStream : public ostream {
	private:
		class LoggingBuffer : public stringbuf {
			private:
				LoggingLevelData m_data;
			public:
				LoggingBuffer(const LoggingLevelData& data) : m_data(data) {}

				~LoggingBuffer() {
					pubsync();
				}

				int sync() {
					if (Logger::getLoggingLevel() <= m_data.level) {
						m_data.stream << m_data.name << ": " << str();
					}
					str("");
					return !m_data.stream;
				}
		};

	public:
		LoggingStream(const LoggingLevelData& data) : ostream(new LoggingBuffer(data)) {}
		~LoggingStream() { delete rdbuf(); }
};

#ifdef DEBUG 
int Logger::logging_level = LOG_LEVEL_ALL;
#else
int Logger::logging_level = LOG_LEVEL_WARN;
#endif

LoggingStream debug_stream({ LOG_LEVEL_DEBUG, "Debug", cout });
LoggingStream info_stream({ LOG_LEVEL_INFO, "Info", cout });
LoggingStream warn_stream({ LOG_LEVEL_WARN, "Warning", cout });
LoggingStream error_stream({ LOG_LEVEL_ERROR, "Error", cerr });

ostream& Logger::debug = debug_stream;
ostream& Logger::info = info_stream;
ostream& Logger::warn = warn_stream;
ostream& Logger::error = error_stream;

void Logger::setLoggingLevel(int new_level) {
	logging_level = new_level;
}

int Logger::getLoggingLevel() {
	return logging_level;
}
