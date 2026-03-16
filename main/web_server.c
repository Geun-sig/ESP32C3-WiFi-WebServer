#include "web_server.h"
#include "adc_reader.h"
#include <string.h>
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_log.h"

static const char *TAG = "web_server";

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
    len += snprintf(json + len, sizeof(json) - len, "],\"adc\":%d}", adc_reader_get());

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, len);
    return ESP_OK;
}

static esp_err_t adc_get_handler(httpd_req_t *req)
{
    char json[32];
    int len = snprintf(json, sizeof(json), "{\"adc\":%d}", adc_reader_get());

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, len);
    return ESP_OK;
}

void start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(httpd_start(&server, &config));

    const httpd_uri_t uris[] = {
        { .uri = "/",           .method = HTTP_GET, .handler = hello_get_handler  },
        { .uri = "/style.css",  .method = HTTP_GET, .handler = style_get_handler  },
        { .uri = "/script.js",  .method = HTTP_GET, .handler = script_get_handler },
        { .uri = "/api/status", .method = HTTP_GET, .handler = status_get_handler },
        { .uri = "/api/adc",    .method = HTTP_GET, .handler = adc_get_handler    },
    };

    for (int i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) {
        httpd_register_uri_handler(server, &uris[i]);
    }

    ESP_LOGI(TAG, "HTTP server started");
}
