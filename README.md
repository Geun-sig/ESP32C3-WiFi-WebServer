# ESP32-C3 WiFi SoftAP Web Dashboard

ESP32-C3(Seeed XIAO ESP32C3) 기반의 WiFi Soft Access Point 프로젝트.
연결된 클라이언트의 RSSI와 ADC 값을 웹 브라우저에서 실시간으로 모니터링할 수 있습니다.

---

## 주요 기능

- WiFi SoftAP 모드 동작 (WPA2-PSK)
- 내장 HTTP 웹서버
- 연결된 클라이언트 RSSI 실시간 라인 그래프
- GPIO2 ADC 값 실시간 라인 그래프 (200ms 갱신)
- 반응형 웹 UI (PC / 태블릿 / 모바일)

---

## 하드웨어 요구사항

| 항목 | 내용 |
|------|------|
| 보드 | Seeed Studio XIAO ESP32-C3 |
| ADC 입력 | GPIO2 (ADC1 CH2) |
| 배터리 | JST 1.25mm 리튬 배터리 (선택사항, 최대 350mA 충전) |

---

## 소프트웨어 요구사항

- [ESP-IDF v5.2 이상](https://docs.espressif.com/projects/esp-idf/en/latest/)
- CMake 3.22 이상

---

## 빌드 및 플래시

```bash
# ESP-IDF 환경 설정
. $IDF_PATH/export.sh

# 빌드
idf.py build

# 플래시 및 모니터
idf.py -p (PORT) flash monitor
```

---

## WiFi 설정

`main/wifi_ap.c` 상단에서 직접 수정:

```c
#define WIFI_SSID    "my_network"
#define WIFI_PASS    "my_password"
#define WIFI_CHANNEL  1
#define MAX_STA_CONN  4
```

- 비밀번호를 `""` 로 설정하면 오픈 네트워크
- 채널 범위: 1 ~ 13

---

## 웹 접속

1. ESP32-C3에 펌웨어 플래시
2. `my_network` WiFi에 접속 (비밀번호: `my_password`)
3. 브라우저에서 `http://192.168.4.1` 접속

---

## 프로젝트 구조

```
softAP/
├── README.md
├── CLAUDE.md
├── CMakeLists.txt
├── docs/
│   ├── API.md
│   └── ARCHITECTURE.md
└── main/
    ├── main.c              # 진입점 (app_main)
    ├── wifi_ap.c/h         # WiFi SoftAP
    ├── web_server.c/h      # HTTP 서버 및 URI 핸들러
    ├── adc_reader.c/h      # ADC 읽기 (GPIO2)
    ├── index.html          # 웹 페이지 구조
    ├── style.css           # 스타일 (반응형)
    └── script.js           # 그래프 및 데이터 갱신
```

---

## 웹 UI 구성

```
┌─────────────────────────────────────────┐
│  Hello, ESP32!   ● Online               │
├──────────────────┬──────────────────────┤
│  WiFi SoftAP     │  WiFi Signal (RSSI)  │
│  정보 테이블      │  [라인 그래프]        │
│                  ├──────────────────────┤
│  접속 안내 노트   │  ADC (GPIO2 / CH2)   │
│                  │  [라인 그래프]        │
└──────────────────┴──────────────────────┘
```

| 항목 | 내용 |
|------|------|
| RSSI 그래프 | 클라이언트별 신호 강도, 3초 갱신, 최대 20샘플 |
| ADC 그래프 | GPIO2 원시값 (0~4095), 200ms 갱신, 최대 20샘플 |
| 반응형 | 768px 이하 1열, 480px 이하 모바일 최적화 |

---

## 관련 문서

- [API 명세](docs/API.md)
- [코드 아키텍처](docs/ARCHITECTURE.md)
