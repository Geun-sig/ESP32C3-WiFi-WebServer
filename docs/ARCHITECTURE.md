# 코드 아키텍처

---

## 모듈 구조

```
┌─────────────────────────────────────────┐
│              main.c                     │
│  - NVS 초기화                            │
│  - 모듈 초기화 순서 조율                  │
└────┬──────────┬──────────────┬──────────┘
     │          │              │
     ▼          ▼              ▼
┌─────────┐ ┌──────────┐ ┌────────────┐
│wifi_ap  │ │web_server│ │ adc_reader │
│         │ │          │ │            │
│- SoftAP │ │- HTTP    │ │- ADC1 CH2  │
│  초기화  │ │  서버    │ │  초기화    │
│- 이벤트 │ │- URI     │ │- 원시값    │
│  핸들러 │ │  핸들러  │ │  읽기      │
└─────────┘ └────┬─────┘ └────────────┘
                 │ (adc_reader_get() 호출)
                 ▼
         ┌──────────────┐
         │  index.html  │
         │  style.css   │  (EMBED_TXTFILES)
         │  script.js   │
         └──────────────┘
```

---

## 파일별 역할

### main.c
- `app_main()` 만 존재
- 초기화 순서: NVS → ADC → WiFi → WebServer

### wifi_ap.c / wifi_ap.h
- WiFi SoftAP 초기화 및 설정
- 클라이언트 접속/해제 이벤트 로깅
- WiFi 설정값 (`SSID`, `PASS`, `CHANNEL`, `MAX_STA_CONN`) 정의

### web_server.c / web_server.h
- HTTP 서버 시작 및 URI 핸들러 등록
- 정적 파일 서빙 (HTML, CSS, JS)
- REST API 처리 (`/api/status`, `/api/adc`)
- `adc_reader.h` 의존

### adc_reader.c / adc_reader.h
- ADC1 Unit 초기화
- GPIO2(CH2), 12bit, Attenuation 11dB 설정
- `adc_reader_get()` 함수로 원시값 반환

---

## 정적 파일 임베드

`EMBED_TXTFILES`를 사용하여 빌드 시 HTML/CSS/JS를 펌웨어에 포함:

```cmake
EMBED_TXTFILES index.html style.css script.js
```

빌드 후 자동 생성되는 심볼:
```c
extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[]   asm("_binary_index_html_end");
```

> **주의:** `EMBED_TXTFILES`는 파일 끝에 null 바이트(`\0`)를 추가합니다.
> JS 파일 전송 시 반드시 `-1` 처리:
> ```c
> httpd_resp_send(req, script_js_start, script_js_end - script_js_start - 1);
> ```

---

## URI 핸들러 등록 방식

배열로 일괄 등록하여 반복 코드 최소화:

```c
const httpd_uri_t uris[] = {
    { .uri = "/",           .handler = hello_get_handler  },
    { .uri = "/style.css",  .handler = style_get_handler  },
    { .uri = "/script.js",  .handler = script_get_handler },
    { .uri = "/api/status", .handler = status_get_handler },
    { .uri = "/api/adc",    .handler = adc_get_handler    },
};
for (int i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) {
    httpd_register_uri_handler(server, &uris[i]);
}
```

---

## 프론트엔드 구조

### 데이터 흐름

```
브라우저
  │
  ├─ [3초마다] GET /api/status ──► RSSI 라인 그래프 갱신
  │                                 (최대 20샘플 유지)
  │
  └─ [200ms마다] GET /api/adc ───► ADC 라인 그래프 갱신
                                   (최대 20샘플 유지)
```

### 파일별 역할

| 파일 | 역할 |
|------|------|
| `index.html` | 페이지 구조, 2열 Grid 레이아웃 |
| `style.css` | 디자인, 반응형 미디어 쿼리 |
| `script.js` | fetch, Canvas 그래프 드로잉 |

### 반응형 브레이크포인트

| 화면 너비 | 레이아웃 |
|-----------|----------|
| 768px 초과 | 2열 Grid |
| 768px 이하 | 1열 (세로 배치) |
| 480px 이하 | 모바일 최적화 (폰트/패딩 축소) |

---

## 의존성

```
main.c
  └── wifi_ap.h
  └── web_server.h
        └── adc_reader.h
  └── adc_reader.h

CMakeLists.txt PRIV_REQUIRES:
  - esp_wifi
  - nvs_flash
  - esp_http_server
  - esp_adc
```

---

## 향후 개선 방향

| 항목 | 설명 |
|------|------|
| WebSocket | 폴링 방식 → 서버 Push로 전환, 실시간성 향상 |
| SVG 그래프 | Canvas API → SVG로 교체, 반응형 및 스타일링 용이 |
| CSS Variables | 색상/크기 변수화로 테마 변경 용이 |
| Dark Mode | `prefers-color-scheme` 미디어 쿼리 대응 |
| NVS 설정 저장 | WiFi SSID/PASS를 NVS에 저장하여 런타임 변경 가능 |
