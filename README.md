# Nextchip-Team-Project
# Nextchip Apache6 기반 실시간 지정차로 단속 및 V2X 관제 시스템

Nextchip Apache6 SoM 보드의 NPU를 활용하여 지정차로 위반 차량을 실시간으로 단속하고, V2X 통신망을 통해 위험 이벤트(급제동 등)를 주변 차량에 초저지연으로 전파하는 프로젝트입니다.

## 🛠 Tech Stack
* **Hardware:** Nextchip Apache6 SoM (12 TOPS NPU 가속기 탑재)
* **OS:** Ubuntu 20.04 LTS (Embedded)
* **Language:** C
* **AI Models:** YOLOv8 (객체 검출), UFLD (차선 인식) - Tri-Chimera 아키텍처
* **Communication:** TCP Socket
* **UI/Visualization:** ncurses (터미널 HUD 모드)

## 🏗 System Architecture
Edge AI 관제 서버와 다수의 터미널 클라이언트 간의 1:N 통신 구조로 설계되었습니다.

1. **Edge AI 관제 서버 (Apache6):** NPU 기반 객체/차선 추론, 위반 판단 로직 처리, pthread 기반 TCP Broadcast 멀티 스레딩.
2. **Network Protocol:** 통신 지연 최소화를 위해 `__attribute__((packed))`가 적용된 16바이트 C 구조체(V2X_Data) 페이로드 기반 바이너리 전송.
3. **터미널 클라이언트:** 수신된 V2X 데이터 실시간 렌더링 및 사용자 긴급 제동(Spacebar) 이벤트 업링크.

## 🚀 Core Features

### 1. 지정차로 위반 감지 (V2I)
* **가상 차로 매핑:** 1~4차로 실시간 분할 및 차종(car, bus, truck, motorcycle) 검출.
* **위반 자동 판별:** YOLOv8 객체 하단 중심점과 UFLD 차선 영역을 대조하여, 차종별 허용 차로 규칙(예: 화물차 4차로) 위반 여부 실시간 판별.

### 2. V2X 급제동 이벤트 전파 (V2V)
* **이벤트 브로드캐스팅:** 선행 차량에서 긴급 제동 발생 시 `event_code == 2`를 생성하여 서버로 송신, 접속된 전체 망으로 즉시 라우팅.
* **능동형 방어 제어:** 이벤트를 수신한 후행 차량 HUD에 경고를 표출하고, 가상 주행 속도를 0km/h로 강제 감속 시뮬레이션.

## 🔧 Engineering Troubleshooting

* **TCP 패킷 파편화(Fragmentation) 완벽 방어**
  * **Issue:** 네트워크 지연 시 16바이트 구조체가 쪼개져 수신되며, `16777216`과 같은 쓰레기 값이 데이터에 삽입되는 현상 발생.
  * **Solution:** 전체 구조체 사이즈에 도달할 때까지 포인터 오프셋을 이동시키며 패킷을 조립하는 고정 길이 누적 수신(`while` 루프) 로직으로 재설계하여 데이터 무결성 100% 확보.

* **AI 인덱스 - V2X 프로토콜 간 데이터 정합성 확보**
  * **Issue:** YOLO 모델의 Raw 출력 인덱스와 V2X 통신 프로토콜 명세 간의 충돌 위험.
  * **Solution:** 서버-클라이언트 직전에 텍스트 기반 문자열 판별 및 필터링 레이어를 구축하여 프로토콜 규격에 맞게 재매핑.

* **UFLD 아키텍처 분석 및 도메인 정합성 피보팅**
  * **Issue:** 모델 내부 앵커 파라미터(`row_anchor_min = 0.2`)의 한계로 인해, 고정 CCTV 시점 적용 시 렌즈 왜곡 및 상단 미스캔 발생.
  * **Solution:** 오픈소스 아키텍처 특성을 분석하여, 스캔 범위에 최적화된 고도와 앙각을 갖춘 관제 시나리오로 전략적 전환.

* **Direct-File Feeding 구조를 통한 광학 노이즈 차단**
  * **Issue:** 외부 카메라 렌즈 왜곡 및 빛 반사로 인한 소형 객체(오토바이) 미검출.
  * **Solution:** 보드 내장 가상 카메라(`v4l2loopback`)로 영상 파일을 직접 디코딩하는 파이프라인을 구축하여 노이즈 원천 차단.

## 👥 Team & Roles
* **정예찬:** TCP 통신 로직 구현 및 코드 통합
* **최명준:** TCP 통신 및 테스트 영상 최적화
* **한선우:** Edge AI 시스템 최적화
* **이은지:** Edge AI 로직 구현
