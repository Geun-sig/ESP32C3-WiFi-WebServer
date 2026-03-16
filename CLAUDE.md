# ESP32 WiFi SoftAP 프로젝트

## 개요
ESP-IDF 기반의 ESP32 WiFi Soft Access Point 예제에 HTTP 웹서버와 실시간 모니터링 대시보드를 추가한 프로젝트.

---

## 프로젝트 구조

```
softAP/
├── CMakeLists.txt
└── main/
    ├── softap_example_main.c   # 메인 C 소스 (WiFi AP + HTTP 서버)
    ├── CMakeLists.txt          # 빌드 설정 (EMBED_TXTFILES 포함)
    ├── Kconfig.projbuild       # menuconfig 설정 항목
    ├── idf_component.yml       # 컴포넌트 의존성
    ├── index.html              # 웹 페이지 구조
    ├── style.css               # 스타일 (반응형 CSS Grid)
    └── script.js               # 동적 동작 (Canvas 그래프, fetch)
```

---

## WiFi 설정

`softap_example_main.c` 상단의 매크로에서 직접 수정:

```c
#define EXAMPLE_ESP_WIFI_SSID      "my_network"
#define EXAMPLE_ESP_WIFI_PASS      "my_password"
#define EXAMPLE_ESP_WIFI_CHANNEL   1
#define EXAMPLE_MAX_STA_CONN       4
```

- 비밀번호를 `""` 로 설정하면 오픈 네트워크로 동작
- menuconfig(`idf.py menuconfig`) 없이 코드에서 직접 설정 가능

---

## HTTP 서버 엔드포인트

| 경로 | 설명 |
|------|------|
| `/` | `index.html` 반환 |
| `/style.css` | `style.css` 반환 |
| `/script.js` | `script.js` 반환 |
| `/api/status` | 연결된 STA 목록 + RSSI JSON 반환 |

### `/api/status` 응답 예시
```json
{"stations":[{"mac":"aa:bb:cc:dd:ee:ff","rssi":-65}]}
```

---

## 웹 페이지 기능

### 레이아웃
- CSS Grid 2열 구조 (좌: 정보 테이블 / 우: 그래프)
- 반응형: 768px 이하 태블릿, 480px 이하 모바일에서 1열로 전환

### 좌측 컬럼
- WiFi SoftAP 정보 테이블 (SSID, Channel, Max Connections, Security, IP)
- 접속 안내 노트
- 페이지 로드 시각 표시

### 우측 컬럼
- **RSSI 라인 그래프**: 연결된 클라이언트별 신호 강도를 시간축으로 표시 (3초 간격 갱신, 최대 20샘플)
- **Random Noise 그래프**: -1.0 ~ 1.0 범위의 랜덤 노이즈를 실시간 표시 (200ms 간격, 최대 60샘플)

---

## 정적 파일 임베드 방식

`EMBED_TXTFILES`를 사용하여 HTML/CSS/JS를 빌드 시 펌웨어에 포함:

```cmake
idf_component_register(SRCS "softap_example_main.c"
                    PRIV_REQUIRES esp_wifi nvs_flash esp_http_server
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES index.html style.css script.js)
```

C 코드에서 참조:
```c
extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[]   asm("_binary_index_html_end");
```

### 주의사항
- `EMBED_TXTFILES`는 파일 끝에 null 바이트(`\0`)를 추가함
- HTML/CSS는 null 바이트를 무시하지만 **JS 파서는 SyntaxError 발생**
- JS 파일 전송 시 반드시 길이에서 `-1` 처리:
  ```c
  httpd_resp_send(req, script_js_start, script_js_end - script_js_start - 1);
  ```

---

## 트러블슈팅 기록

### 1. JS SyntaxError (Invalid or unexpected token)
- **원인**: `EMBED_TXTFILES`가 추가한 null 바이트를 JS 파서가 잘못된 토큰으로 인식
- **해결**: `script_js_end - script_js_start - 1` 로 null 바이트 제외

### 2. `/api/status` fetch 실패
- **원인 1**: `wifi_sta_list_t` 초기화 누락 → `sta_list.num`에 쓰레기 값 → JSON 버퍼 오버플로우
- **해결 1**: `memset(&sta_list, 0, sizeof(sta_list))` 추가
- **원인 2**: JS에서 `var history = []` 선언 → `window.history`(브라우저 내장 객체)와 충돌 → `.push()` 호출 시 TypeError 발생 → `.catch()`에서 오류로 처리
- **해결 2**: `const history = []` 로 변경 (블록 스코프로 window.history와 분리)

---

## 웹 접속 방법

1. ESP32에 펌웨어 플래시
2. `my_network` WiFi에 접속 (비밀번호: `my_password`)
3. 브라우저에서 `http://192.168.4.1` 접속
