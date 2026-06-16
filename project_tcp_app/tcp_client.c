#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "tcp_client.h"

static int sock_fd;
static V2X_Data latest_data;
static int has_new_data = 0;
static pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;

// [백그라운드 스레드] 무한히 서버 데이터만 기다림
static void *receive_loop(void *arg) {
    V2X_Data net_data;
    
    while (1) {
        int n = recv(sock_fd, &net_data, sizeof(V2X_Data), 0);
        if (n <= 0) {
            // 서버가 죽었거나 끊김
            break; 
        }

        // ★ 핵심: 네트워크 바이트(Big) -> 호스트 바이트(Little)로 해독
        pthread_mutex_lock(&data_mutex);
        latest_data.lane = ntohl(net_data.lane);
        latest_data.class_id = ntohl(net_data.class_id);
        latest_data.event_code = ntohl(net_data.event_code);
        latest_data.vehicle_id = ntohl(net_data.vehicle_id);
        has_new_data = 1; // 새로운 데이터 flag
        pthread_mutex_unlock(&data_mutex);
    }
    
    close(sock_fd);
    return NULL;
}

int client_connect(const char *ip, int port) {
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(ip);
    serv_addr.sin_port = htons(port);

    if (connect(sock_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        return -1; // 연결 실패
    }

    // 연결 성공하면 수신 전담 스레드 생성
    pthread_t tid;
    pthread_create(&tid, NULL, receive_loop, NULL);
    pthread_detach(tid); 
    
    return 0;
}

// 메인 UI 루프에서 주기적으로 확인하는 함수
int get_latest_v2x_data(V2X_Data *out_data) {
    int ret = 0;
    pthread_mutex_lock(&data_mutex);
    if (has_new_data) {
        *out_data = latest_data; // 데이터 복사해서 넘겨줌
        has_new_data = 0;        // flag down
        ret = 1;
    }
    pthread_mutex_unlock(&data_mutex);
    return ret;
}

int client_send_v2x_data(V2X_Data *my_data) {
    V2X_Data net_data;
    // 서버로 보낼 때도 네트워크 바이트(Big-Endian) 변환 필수
    net_data.lane = htonl(my_data->lane);
    net_data.class_id = htonl(my_data->class_id);
    net_data.event_code = htonl(my_data->event_code);

    if (send(sock_fd, &net_data, sizeof(V2X_Data), 0) < 0) {
        return -1;
    }
    return 0;
}