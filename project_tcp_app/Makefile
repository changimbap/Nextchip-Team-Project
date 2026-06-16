CC_HOST   = gcc
CC_TARGET = aarch64-none-linux-gnu-gcc
CFLAGS    = -Wall -O2
LDFLAGS   = -lpthread -lncursesw

SERVER_BIN = v2x_server
CLIENT_BIN = v2x_client
DASHBOARD_BIN = v2x_dashboard

# all 에 DASHBOARD_BIN 추가
all: $(SERVER_BIN) $(CLIENT_BIN) $(DASHBOARD_BIN)

$(SERVER_BIN): main.c tcp_server.c
	$(CC_TARGET) $(CFLAGS) -pthread -o $(SERVER_BIN) main.c tcp_server.c

$(CLIENT_BIN): client_main.c tcp_client.c
	$(CC_HOST) $(CFLAGS) -o $(CLIENT_BIN) client_main.c tcp_client.c $(LDFLAGS)

$(DASHBOARD_BIN): dashboard_main.c tcp_client.c
	$(CC_HOST) $(CFLAGS) -o $(DASHBOARD_BIN) dashboard_main.c tcp_client.c $(LDFLAGS)
	@echo "🟢 [SUCCESS] 전광판 클라이언트 컴파일 완료!"

clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) $(DASHBOARD_BIN)