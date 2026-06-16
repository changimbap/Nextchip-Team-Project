#ifndef V2X_PROTOCOL_H
#define V2X_PROTOCOL_H

#include <stdint.h>

// 네트워크 통신 시 크기 오류를 막기 위해 크기가 고정된 int32_t 사용
typedef struct {
    int32_t lane;        // 차선 (1, 2, 3...)
    int32_t class_id;    // 0: 승용차, 1: 트럭, 2: 오토바이
    int32_t event_code;  // 0: 정상, 1: 지정차로 위반, 2: 급제동 경고
    int32_t vehicle_id;
} V2X_Data;

#endif // V2X_PROTOCOL_H