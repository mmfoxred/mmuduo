#pragma once
#include <string>

#include "noncopyable.h"

#define LOG_INFO(msgFormat, ...)                       \
    do {                                               \
        Logger& logger = Logger::getInstance();        \
        logger.setLogLevel(LOGGERLEVEL::INFO);         \
        char buf[1024] = {0};                          \
        snprintf(buf, 1024, msgFormat, ##__VA_ARGS__); \
        logger.log(buf);                               \
    } while (0)
#define LOG_ERROR(msgFormat, ...)                      \
    do {                                               \
        Logger& logger = Logger::getInstance();        \
        logger.setLogLevel(LOGGERLEVEL::ERROR);        \
        char buf[1024] = {0};                          \
        snprintf(buf, 1024, msgFormat, ##__VA_ARGS__); \
        logger.log(buf);                               \
    } while (0)
#define LOG_FATAL(msgFormat, ...)                      \
    do {                                               \
        Logger& logger = Logger::getInstance();        \
        logger.setLogLevel(LOGGERLEVEL::FATAL);        \
        char buf[1024] = {0};                          \
        snprintf(buf, 1024, msgFormat, ##__VA_ARGS__); \
        logger.log(buf);                               \
    } while (0)
#ifdef MUDEBUG
#define LOG_DEBUG(msgFormat, ...)                      \
    do {                                               \
        Logger& logger = Logger::getInstance();        \
        logger.setLogLevel(LOGGERLEVEL::DEBUG);        \
        char buf[1024] = {0};                          \
        snprintf(buf, 1024, msgFormat, ##__VA_ARGS__); \
        logger.log(buf);                               \
    } while (0)
#else
#define LOG_DEBUG(msgFormat, ...)
#endif

enum LOGGERLEVEL { INFO, ERROR, DEBUG, FATAL };

class Logger : private noncopyable {
public:
    static Logger& getInstance();
    void setLogLevel(int level);
    void log(std::string msg)const ;

private:
    int m_logLevel;
};