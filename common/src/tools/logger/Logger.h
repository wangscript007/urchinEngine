#ifndef URCHINENGINE_LOGGER_H
#define URCHINENGINE_LOGGER_H

#include <string>
#include <sstream>
#include <iostream>

namespace urchin
{
	
	class Logger
	{
		public:
			virtual ~Logger();

			enum CriticalityLevel
			{
				INFO,
				WARNING,
				ERROR
			};
			
			static Logger& logger();

			void logInfo(const std::string &);
			void logWarning(const std::string &);
			void logError(const std::string &);

			void log(CriticalityLevel, const std::string &);

		private:
			std::string prefix(CriticalityLevel);
			std::string getCriticalityString(CriticalityLevel);

			virtual void write(const std::string &) = 0;

			static Logger *instance;
	};

}

#endif
