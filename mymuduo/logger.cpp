#include "logger.h"

#include <iostream>

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLogLevel(int level) { this->m_logLevel = level; }

void Logger::log(std::string msg) {
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
    std::cout << "print time " << msg;  // 格式化应该留给调用者
}

// int main() {
//     Logger logger;
//     LOG_INFO("%s", "hello,test loginfo\n");
//     LOG_ERROR("%s,%d", "hello,test loginfo\n", LOGGERLEVEL::ERROR);
//     LOG_FATAL("%s,%d", "hello,test loginfo\n", LOGGERLEVEL::FATAL);
//     return 0;
// }