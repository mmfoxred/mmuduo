#pragma once
#include <string>
class Timestamp {
public:
    Timestamp();
    Timestamp(long secondsSinceEpoch);
    static Timestamp now();

    bool valid() const { return m_microSecondsSinceEpoch > 0; }
    static Timestamp invalid() { return Timestamp(); }
    const std::string toString() const;

    static const int kMicroSecondsPerSecond = 1000 * 1000;
    int64_t microSecondsSinceEpoch() const { return m_microSecondsSinceEpoch; }
    time_t secondsSinceEpoch() const {
        return static_cast<time_t>(m_microSecondsSinceEpoch /
                                   kMicroSecondsPerSecond);
    }

private:
    long m_secondSinceEpoch;
    long m_microSecondsSinceEpoch;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() < rhs.microSecondsSinceEpoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
    return lhs.microSecondsSinceEpoch() == rhs.microSecondsSinceEpoch();
}

inline Timestamp addTime(Timestamp timestamp, double seconds) {
    int64_t delta =
        static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
    return Timestamp(timestamp.microSecondsSinceEpoch() + delta);
}