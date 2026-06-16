#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include "v2x_protocol.h"

// 서버 초기화 및 백그라운드 수신 대기(Accept) 스레드 실행
void server_init(int port);

// 접속한 모든 클라이언트에게 V2X_Data 구조체를 쏘는 함수 (hton 변환 포함)
void broadcast_v2x_data(V2X_Data *data);

#endif // TCP_SERVER_H