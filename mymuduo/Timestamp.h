#pragma once
#include <string>
class Timestamp {
public:
    Timestamp();
    Timestamp(long secondsSinceEpoch);
    static Timestamp now();
    const std::string toString() const;

private:
    long m_secondSinceEpoch;
};