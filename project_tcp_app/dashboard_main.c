#include <locale.h>
#include <ncurses.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "tcp_client.h"

#define MAX_LOGS 10 // 화면에 표시할 최대 로그 개수

// 로그를 저장할 구조체 (내부용)
typedef struct {
    int lane;
    int class_id;
    int vehicle_id;
} ViolationLog;

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("🚨 사용법: %s [서버IP] [포트]\n", argv[0]);
        return -1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char *class_names[] = {"승용차", "화물차", "오토바이"};

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
    init_pair(1, COLOR_GREEN, COLOR_BLACK);  
    init_pair(2, COLOR_RED, COLOR_BLACK);    
    init_pair(3, COLOR_YELLOW, COLOR_BLACK); 

    V2X_Data incoming_data;
    int violation_detected = 0;
    int display_timer = 0;
    
    // 위반 기록 저장용 배열 및 카운터
    ViolationLog logs[MAX_LOGS];
    int log_count = 0;
    
    // 현재 알람 팝업용 변수
    int viol_lane = 0;
    int viol_class = 0;
    int viol_id = 0;

    while (1) {
        if (get_latest_v2x_data(&incoming_data)) {
            // 위반 이벤트(1)가 발생했을 때
            if (incoming_data.event_code == 1) { 
                violation_detected = 1;
                display_timer = 30; // 3초간 상단 팝업 유지
                viol_lane = incoming_data.lane;
                viol_class = incoming_data.class_id;
                viol_id = incoming_data.vehicle_id;

                // ⭐ [로그 업데이트 로직] 
                // 기존 로그들을 한 칸씩 밑으로 밀어냄 (최신순 정렬)
                for (int i = MAX_LOGS - 1; i > 0; i--) {
                    logs[i] = logs[i-1];
                }
                
                // 맨 앞(인덱스 0)에 새로운 위반 차량 정보 저장
                logs[0].lane = viol_lane;
                logs[0].class_id = viol_class;
                logs[0].vehicle_id = viol_id;
                
                if (log_count < MAX_LOGS) log_count++;
            }
        }

        clear();
        attron(COLOR_PAIR(3));
        mvprintw(1, 5, "==========================================================");
        mvprintw(2, 5, "            🚦 도로 상황 종합 관제 사스템                 ");
        mvprintw(3, 5, "==========================================================");
        attroff(COLOR_PAIR(3));

        mvprintw(5, 5, "📡 시스템 상태: 정상 가동 중 (Connected: %s:%d)", server_ip, port);

        // 1. [상단] 실시간 단속 팝업 알림창
        if (violation_detected && display_timer > 0) {
            attron(COLOR_PAIR(2));
            mvprintw(7,  5, "┏━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓");
            mvprintw(8,  5, "┃ 🚨 [실시간 단속] 지정차로 위반 차량 발견!          ┃");
            mvprintw(9,  5, "┃  - 차선: %d 차로                                   ┃", viol_lane);
            mvprintw(10, 5, "┃  - 차종: %-8s (ID: %-4d)                       ┃", class_names[viol_class], viol_id);
            mvprintw(11, 5, "┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┛");
            attroff(COLOR_PAIR(2));
            display_timer--;
        } else {
            attron(COLOR_PAIR(1));
            mvprintw(9, 5, "✅ 현재 도로 상황: 안전 (위반 차량 없음)");
            attroff(COLOR_PAIR(1));
            violation_detected = 0;
        }

        // 2. ⭐ [하단] 최근 10건 단속 로그 리스트 출력
        mvprintw(14, 5, "[ 📋 최근 지정차로 단속 로그 (최대 10건) ]");
        mvprintw(15, 5, "--------------------------------------------------");
        
        for (int i = 0; i < log_count; i++) {
            // 최신 로그(i=0)는 빨간색으로, 나머지는 흰색으로 하이라이트!
            if (i == 0 && display_timer > 0) attron(COLOR_PAIR(2));
            
            mvprintw(16 + i, 5, "%2d. [ID: %4d] 🚨 %d차로 - %s 위반 적발", 
                     i + 1, logs[i].vehicle_id, logs[i].lane, class_names[logs[i].class_id]);
                     
            if (i == 0 && display_timer > 0) attroff(COLOR_PAIR(2));
        }
        mvprintw(16 + log_count + 2, 5, "종료하려면 'q'를 누르세요.");

        refresh();

        if (getch() == 'q') break;
        usleep(100000); 
    }

    endwin();
    return 0;
}