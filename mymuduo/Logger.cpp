#include "Logger.h"

#include <iostream>

#include "Timestamp.h"

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLogLevel(int level) { this->m_logLevel = level; }

void Logger::log(std::string msg) const {
    switch (m_logLevel) {
        case LOGGERLEVEL::INFO:
            std::cout << "[INFO] ";
            break;
        case LOGGERLEVEL::ERROR:
            std::cout << "[ERROR] ";
            break;
        case LOGGERLEVEL::DEBUG:
            std::cout << "[DEBUG] ";
            break;
        case LOGGERLEVEL::FATAL:
            std::cout << "[FATAL] ";
            break;
    }
    std::cout << Timestamp::now().toString() << msg;  // 格式化应该留给调用者
}

// int main() {
//     Logger logger;
//     LOG_INFO("%s\n", "hello,test loginfo");
//     LOG_ERROR("%s,%d\n", "hello,test loginfo", LOGGERLEVEL::ERROR);
//     LOG_FATAL("%s,%d\n", "hello,test loginfo", LOGGERLEVEL::FATAL);
//     return 0;
// }