#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include "tcp_server.h"

#define MAX_CLIENTS 32

typedef struct {
    int fd;
    struct sockaddr_in addr;
} client_t;

// 접속자 관리 배열 및 뮤텍스 (스레드 충돌 방지)
static client_t *clients[MAX_CLIENTS];
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
static int listen_fd;

// ---------------------------------------------------------
// [내부 함수] 클라이언트 배열 관리
// ---------------------------------------------------------
static void add_client(client_t *cl) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i]) { clients[i] = cl; break; }
    }
    pthread_mutex_unlock(&clients_mutex);
}

static void remove_client(int fd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && clients[i]->fd == fd) { 
            clients[i] = NULL; 
            break; 
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

// ---------------------------------------------------------
// [내부 함수] 각 클라이언트 연결 유지용 스레드
// V2X는 서버가 쏘기만 하므로, 여기서는 클라이언트가 끊겼는지만 감시함
// ---------------------------------------------------------
static void *handle_client(void *arg) {
    client_t *cl = (client_t *)arg;
    V2X_Data incoming_data;
    
    printf("[서버] 새로운 차량 접속됨 (fd: %d)\n", cl->fd);

    // recv()는 클라이언트가 강제 종료하거나 끊어질 때 0 또는 -1을 반환함
    while (recv(cl->fd, &incoming_data, sizeof(V2X_Data), 0) > 0) {

        V2X_Data host_data;
        host_data.lane = ntohl(incoming_data.lane);
        host_data.class_id = ntohl(incoming_data.class_id);
        host_data.event_code = ntohl(incoming_data.event_code);

        printf("🚨 [V2V 통신] 차량(fd:%d)이 위험 경보 전파 시작\n", cl->fd);

        broadcast_v2x_data(&host_data);
    }

    printf("[서버] 차량(단말기) 연결 끊김 (fd: %d)\n", cl->fd);
    remove_client(cl->fd);
    close(cl->fd);
    free(cl);
    return NULL;
}

// ---------------------------------------------------------
// [내부 함수] 클라이언트 접속(Accept) 대기 무한 루프 스레드
// 메인 AI 로직이 멈추지 않도록 백그라운드에서 돌아감
// ---------------------------------------------------------
static void *accept_loop(void *arg) {
    (void)arg;
    while (1) {
        client_t *cl = (client_t *)calloc(1, sizeof(client_t));
        socklen_t len = sizeof(cl->addr);
        
        cl->fd = accept(listen_fd, (struct sockaddr *)&cl->addr, &len);
        if (cl->fd < 0) {
            perror("accept error");
            free(cl);
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, cl);
        pthread_detach(tid); // 스레드 자원 자동 회수
        add_client(cl);
    }
    return NULL;
}

// ---------------------------------------------------------
// [외부 호출용 API 1] 서버 초기화
// ---------------------------------------------------------
void server_init(int port) {
    signal(SIGPIPE, SIG_IGN); // 클라이언트 튕겼을 때 서버 죽는 현상 방지

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons((uint16_t)port);

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind error");
        exit(1);
    }
    
    if (listen(listen_fd, 10) < 0) {
        perror("listen error");
        exit(1);
    }

    printf("[서버] V2X 관제 서버 가동 시작 (Port: %d)\n", port);

    // Accept를 처리할 백그라운드 스레드 생성
    pthread_t accept_tid;
    pthread_create(&accept_tid, NULL, accept_loop, NULL);
    pthread_detach(accept_tid);
}

// ---------------------------------------------------------
// [외부 호출용 API 2] 구조체 브로드캐스트
// ---------------------------------------------------------
void broadcast_v2x_data(V2X_Data *data) {
    V2X_Data net_data;
    
    // 가장 중요한 부분: 호스트 바이트(PC) -> 네트워크 바이트(통신망) 변환
    net_data.lane = htonl(data->lane);
    net_data.class_id = htonl(data->class_id);
    net_data.event_code = htonl(data->event_code);
    net_data.vehicle_id = htonl(data->vehicle_id);

    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] != NULL) {
            // 변환된 구조체를 통째로 메모리 전송
            if (send(clients[i]->fd, &net_data, sizeof(V2X_Data), 0) < 0) {
                // 전송 실패 시 에러 무시 (다음 턴에 알아서 끊김 처리됨)
            }
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}