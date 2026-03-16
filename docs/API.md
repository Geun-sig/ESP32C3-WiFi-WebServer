# HTTP API 명세

Base URL: `http://192.168.4.1`

---

## 정적 파일

### GET /

웹 대시보드 메인 페이지를 반환합니다.

- **Response:** `text/html`
- **Body:** `index.html` 내용

---

### GET /style.css

페이지 스타일시트를 반환합니다.

- **Response:** `text/css`
- **Body:** `style.css` 내용

---

### GET /script.js

페이지 JavaScript를 반환합니다.

- **Response:** `application/javascript; charset=utf-8`
- **Body:** `script.js` 내용

---

## 데이터 API

### GET /api/status

연결된 클라이언트 목록과 RSSI, ADC 값을 반환합니다.

- **Response:** `application/json`
- **갱신 주기:** 3초 (클라이언트 측 폴링)

**응답 예시:**
```json
{
  "stations": [
    { "mac": "aa:bb:cc:dd:ee:ff", "rssi": -65 },
    { "mac": "11:22:33:44:55:66", "rssi": -72 }
  ],
  "adc": 2048
}
```

**필드 설명:**

| 필드 | 타입 | 설명 |
|------|------|------|
| `stations` | array | 연결된 STA 목록 |
| `stations[].mac` | string | 클라이언트 MAC 주소 |
| `stations[].rssi` | integer | 신호 강도 (dBm, 범위: -100 ~ 0) |
| `adc` | integer | ADC 원시값 (범위: 0 ~ 4095) |

---

### GET /api/adc

GPIO2(ADC1 CH2)의 현재 ADC 값만 반환합니다.

- **Response:** `application/json`
- **갱신 주기:** 200ms (클라이언트 측 폴링)

**응답 예시:**
```json
{ "adc": 2048 }
```

**필드 설명:**

| 필드 | 타입 | 설명 |
|------|------|------|
| `adc` | integer | ADC 원시값 (범위: 0 ~ 4095, 12bit) |

---

## RSSI 신호 강도 기준

| 범위 | 색상 | 품질 |
|------|------|------|
| -60dBm 이상 | 초록 | 강함 |
| -75 ~ -60dBm | 주황 | 보통 |
| -75dBm 미만 | 빨강 | 약함 |
