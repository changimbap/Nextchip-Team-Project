#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "tcp_server.h"

int main() {
    srand(time(NULL));

    // 1. 서버 열기 (포트 5000번)
    server_init(5000);

    // 2. 가짜 AI 루프 (3초마다 데이터 생성해서 쏨)
    while (1) {
        sleep(3);

        V2X_Data mock_data;
        mock_data.lane = (rand() % 3) + 1;       // 1~3차선 랜덤
        mock_data.class_id = rand() % 3;         // 0:승용, 1:트럭, 2:오토바이
        mock_data.event_code = rand() % 3;       // 0:정상, 1:위반, 2:급제동
        mock_data.vehicle_id = rand() % 9000 + 1000;       // 0:정상, 1:위반, 2:급제동

        printf("\n[AI 탐지] 차선:%d, 차종:%d, 이벤트:%d -> 전송!\n", 
               mock_data.lane, mock_data.class_id, mock_data.event_code);

        // 접속한 차량들에게 데이터 발사!
        broadcast_v2x_data(&mock_data);
    }

    return 0;
}