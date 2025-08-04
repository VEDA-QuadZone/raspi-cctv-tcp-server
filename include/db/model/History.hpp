#ifndef HISTORY_HPP
#define HISTORY_HPP

#include <string>
#include <optional>
struct History {
    std::string date;         // "YYYY-MM-DD HH:MM:SS"
    std::string imagePath;    // 예: "/images/event_001.jpg"
    std::string plateNumber;  // 예: "12가3456"
    int eventType;            // 0: 속도위반, 1: 불법주정차, 2: 어린이감지
    std::string startSnapshot;
    std::string endSnapshot;
    //float speed=-1;
    std::optional<float> speed; // 속도는 선택적, 없을 수도 있음
    int id = -1;
};

#endif // HISTORY_HPP
