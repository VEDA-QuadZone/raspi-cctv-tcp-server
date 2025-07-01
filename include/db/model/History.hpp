#ifndef HISTORY_HPP
#define HISTORY_HPP

#include <string>

struct History {
    int id = -1;
    std::string date;         // "YYYY-MM-DD HH:MM:SS"
    std::string imagePath;    // 예: "/images/event_001.jpg"
    std::string plateNumber;  // 예: "12가3456"
    int eventType;            // 0: 속도위반, 1: 불법주정차, 2: 어린이감지
};

#endif // HISTORY_HPP
