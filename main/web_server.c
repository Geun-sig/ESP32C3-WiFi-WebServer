#include "web_server.h"
#include "adc_reader.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_log.h"

static const char *TAG = "web_server";
static httpd_handle_t server_handle = NULL;
static volatile int ws_interval_ms = 500;

extern const char index_html_start[] asm("_binary_index_html_start");
extern const char index_html_end[]   asm("_binary_index_html_end");
extern const char style_css_start[]  asm("_binary_style_css_start");
extern const char style_css_end[]    asm("_binary_style_css_end");
extern const char script_js_start[]  asm("_binary_script_js_start");
extern const char script_js_end[]    asm("_binary_script_js_end");

static esp_err_t hello_get_handler(httpd_req_t *req)
{
    httpd_resp_send(req, index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

static esp_err_t style_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, style_css_start, style_css_end - style_css_start);
    return ESP_OK;
}

static esp_err_t script_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/javascript; charset=utf-8");
    httpd_resp_send(req, script_js_start, script_js_end - script_js_start - 1);
    return ESP_OK;
}

static esp_err_t status_get_handler(httpd_req_t *req)
{
    wifi_sta_list_t sta_list;
    memset(&sta_list, 0, sizeof(sta_list));
    esp_wifi_ap_get_sta_list(&sta_list);

    char json[512];
    int len = 0;
    len += snprintf(json + len, sizeof(json) - len, "{\"stations\":[");
    for (int i = 0; i < sta_list.num; i++) {
        if (i > 0) len += snprintf(json + len, sizeof(json) - len, ",");
        len += snprintf(json + len, sizeof(json) - len,
            "{\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\",\"rssi\":%d}",
            sta_list.sta[i].mac[0], sta_list.sta[i].mac[1],
            sta_list.sta[i].mac[2], sta_list.sta[i].mac[3],
            sta_list.sta[i].mac[4], sta_list.sta[i].mac[5],
            sta_list.sta[i].rssi);
    }
    len += snprintf(json + len, sizeof(json) - len, "],\"adc\":%d,\"voltage\":%.3f}",
                    adc_reader_get(), adc_reader_get_voltage());

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, len);
    return ESP_OK;
}

static esp_err_t adc_get_handler(httpd_req_t *req)
{
    char json[64];
    int len = snprintf(json, sizeof(json), "{\"adc\":%d,\"voltage\":%.3f}",
                       adc_reader_get(), adc_reader_get_voltage());

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, len);
    return ESP_OK;
}

static esp_err_t ws_handler(httpd_req_t *req)
{
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WS client connected, fd=%d", httpd_req_to_sockfd(req));
        return ESP_OK;
    }

    httpd_ws_frame_t frame;
    memset(&frame, 0, sizeof(frame));

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK || frame.len == 0) return ESP_OK;

    uint8_t *buf = calloc(1, frame.len + 1);
    if (!buf) return ESP_ERR_NO_MEM;
    frame.payload = buf;

    ret = httpd_ws_recv_frame(req, &frame, frame.len);
    if (ret == ESP_OK) {
        int interval = 0;
        if (sscanf((char *)buf, "{\"interval\":%d}", &interval) == 1) {
            if (interval >= 100 && interval <= 5000) {
                ws_interval_ms = interval;
                ESP_LOGI(TAG, "WS interval set to %dms", ws_interval_ms);
            }
        }
    }

    free(buf);
    return ESP_OK;
}

static void ws_broadcast_task(void *arg)
{
    char json[512];

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(ws_interval_ms));
        if (!server_handle) continue;

        wifi_sta_list_t sta_list;
        memset(&sta_list, 0, sizeof(sta_list));
        esp_wifi_ap_get_sta_list(&sta_list);

        int len = 0;
        len += snprintf(json + len, sizeof(json) - len, "{\"stations\":[");
        for (int i = 0; i < sta_list.num; i++) {
            if (i > 0) len += snprintf(json + len, sizeof(json) - len, ",");
            len += snprintf(json + len, sizeof(json) - len,
                "{\"mac\":\"%02x:%02x:%02x:%02x:%02x:%02x\",\"rssi\":%d}",
                sta_list.sta[i].mac[0], sta_list.sta[i].mac[1],
                sta_list.sta[i].mac[2], sta_list.sta[i].mac[3],
                sta_list.sta[i].mac[4], sta_list.sta[i].mac[5],
                sta_list.sta[i].rssi);
        }
        len += snprintf(json + len, sizeof(json) - len, "],\"adc\":%d,\"voltage\":%.3f}",
                        adc_reader_get(), adc_reader_get_voltage());

        size_t clients = 10;
        int client_fds[10];
        if (httpd_get_client_list(server_handle, &clients, client_fds) != ESP_OK) continue;

        for (size_t i = 0; i < clients; i++) {
            if (httpd_ws_get_fd_info(server_handle, client_fds[i]) == HTTPD_WS_CLIENT_WEBSOCKET) {
                httpd_ws_frame_t frame = {
                    .final      = true,
                    .fragmented = false,
                    .type       = HTTPD_WS_TYPE_TEXT,
                    .payload    = (uint8_t *)json,
                    .len        = len,
                };
                httpd_ws_send_frame_async(server_handle, client_fds[i], &frame);
            }
        }
    }
}

static esp_err_t error_404_handler(httpd_req_t *req, httpd_err_code_t err)
{
    const char *resp =
        "<html><head><meta charset='UTF-8'><title>404</title>"
        "<style>body{font-family:Arial,sans-serif;text-align:center;padding:60px;background:#f0f4f8;}"
        "h1{font-size:4rem;color:#e74c3c;margin:0;}p{color:#7f8c8d;}"
        "a{color:#3498db;text-decoration:none;}</style></head>"
        "<body><h1>404</h1><p>Page not found.</p>"
        "<a href='/'>Back to Dashboard</a></body></html>";
    httpd_resp_set_status(req, "404 Not Found");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

void start_webserver(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_open_sockets = 7;
    ESP_ERROR_CHECK(httpd_start(&server_handle, &config));

    const httpd_uri_t uris[] = {
        { .uri = "/",           .method = HTTP_GET, .handler = hello_get_handler  },
        { .uri = "/style.css",  .method = HTTP_GET, .handler = style_get_handler  },
        { .uri = "/script.js",  .method = HTTP_GET, .handler = script_get_handler },
        { .uri = "/api/status", .method = HTTP_GET, .handler = status_get_handler },
        { .uri = "/api/adc",    .method = HTTP_GET, .handler = adc_get_handler    },
        { .uri = "/ws",         .method = HTTP_GET, .handler = ws_handler,
          .is_websocket = true                                                     },
    };

    for (int i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) {
        httpd_register_uri_handler(server_handle, &uris[i]);
    }

    httpd_register_err_handler(server_handle, HTTPD_404_NOT_FOUND, error_404_handler);

    xTaskCreate(ws_broadcast_task, "ws_broadcast", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "HTTP server started");
}
