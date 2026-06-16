#ifndef TCP_CLIENT_H
#define TCP_CLIENT_H

#include "v2x_protocol.h"

// 서버 IP와 포트로 접속하고 백그라운드 수신 스레드 시작
int client_connect(const char *ip, int port);

// 새로 수신된 데이터가 있으면 1 반환 + 데이터 복사, 없으면 0 반환
int get_latest_v2x_data(V2X_Data *out_data);

#endif // TCP_CLIENT_H