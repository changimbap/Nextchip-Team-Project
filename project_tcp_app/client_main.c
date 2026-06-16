#include <locale.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "tcp_client.h"

int main(int argc, char *argv[]) {
    if (argc != 5) {
        printf("🚨 사용법: %s [서버IP] [포트] [내_차선(1~3)] [차종(0:승용, 1:트럭, 2:오토바이)]\n", argv[0]);
        return -1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    int my_lane = atoi(argv[3]);
    int my_class = atoi(argv[4]);
    char *class_names[] = {"승용차", "트럭", "오토바이"};

    if (client_connect(server_ip, port) < 0) {
        printf("❌ 서버 연결 실패!\n");
        return -1;
    }

    setlocale(LC_ALL, ""); 
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE); 
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);  // 정상
    init_pair(2, COLOR_RED, COLOR_BLACK);    // AI 단속 경고 (빨강)
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); // V2V 급제동 경고 (노랑)

    int virtual_speed = 80;
    int warning_mode = 0; // 0:정상, 1:AI단속, 2:V2V급제동
    int warning_timer = 0; // 경고를 몇 프레임 동안 유지할지

    V2X_Data incoming_data;

    while (1) {
        // 1. [수신] 서버에서 날아온 데이터가 있는지 확인
        if (get_latest_v2x_data(&incoming_data)) {
            // 나랑 같은 차선에서 일어난 이벤트일 때만 반응
            if (incoming_data.lane == my_lane) {
                if (incoming_data.event_code == 1) {
                    warning_mode = 1; // AI 관제탑의 단속 경고
                    warning_timer = 30; // 3초 유지
                } else if (incoming_data.event_code == 2) {
                    warning_mode = 2; // 다른 차량의 급제동 경고
                    warning_timer = 30; 
                }
            }
        }

        // 2. [키 입력 & 송신] 스페이스바를 누르면 내가 서버로 경고를 쏨
        int ch = getch();
        if (ch == 'q') break;
        if (ch == ' ') { // 스페이스바 (ASCII 32)
            V2X_Data my_emergency;
            my_emergency.lane = my_lane;
            my_emergency.class_id = my_class;
            my_emergency.event_code = 2; // 2: 급제동 이벤트 코드

            // 서버로 내 긴급 상황 전송!
            client_send_v2x_data(&my_emergency);
            
            // 내 화면도 즉시 급제동 모드로 변경
            warning_mode = 2;
            warning_timer = 30;
            virtual_speed = 0; // 내 속도 0으로
        }

        // 타이머 감소 및 정상 상태 복귀 로직
        if (warning_timer > 0) {
            warning_timer--;
        } else {
            warning_mode = 0; // 타이머 끝나면 정상으로
        }

        // 3. [화면 렌더링]
        clear();
        int color = (warning_mode == 1) ? 2 : (warning_mode == 2 ? 3 : 1);
        attron(COLOR_PAIR(color));
        
        mvprintw(1, 2, "========== V2X SMART DASHBOARD ==========");
        mvprintw(3, 2, "🌐 접속 서버: %s:%d", server_ip, port);
        mvprintw(4, 2, "🚗 내 차량: [%d 차선] %s", my_lane, class_names[my_class]);
        mvprintw(6, 2, "⏱️  가상 속도: %d km/h", virtual_speed);

        if (warning_mode == 1) {
            mvprintw(8, 2, "🚨 [AI 단속] 지정차로 위반 감지! 감속하세요!");
            if (virtual_speed > 60) virtual_speed -= 2; 
        } else if (warning_mode == 2) {
            mvprintw(8, 2, "💥 [V2V 경고] 전방 차량 급제동! 충돌 주의!");
            virtual_speed = 0; // V2V 경고 시에는 즉시 정지!
        } else {
            mvprintw(8, 2, "✅ 주행 상태: 안전 (정상 주행 중)");
            if (virtual_speed < 80) virtual_speed += 1; // 서서히 80km/h로 복귀
        }
        
        mvprintw(11, 2, "🕹️  조작: [Space] 급제동 발송 / [q] 종료");
        attroff(COLOR_PAIR(color));
        refresh();

        usleep(100000); // 0.1초 딜레이 (10 FPS)
    }

    endwin();
    return 0;
}