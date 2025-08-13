#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "cJSON.h"
#include "esp_timer.h"

#define SERVER_IP   "192.168.137.1"   // server IP
#define SERVER_PORT 5000
int64_t offset = 0;
static const char *TAG = "TCP_CLIENT"; // tag for esplog (you will see when you monitor)
void tcp_client_task(void *pvParameters) {
    char rx_buffer[1024];
    struct sockaddr_in dest_addr; // the struct with (sin.family, sin.port, sin.addr) inside, for communication

    //build a TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno); // errno: global variable to save the last error code
        vTaskDelete(NULL);
        return;
    }

    //assign value for dest_addr
    dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);

    //to build a connection, if success return 0
    ESP_LOGI(TAG, "Connecting to %s:%d...", SERVER_IP, SERVER_PORT);
    int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket connect failed: errno %d", errno);
        close(sock);
        vTaskDelete(NULL);
        return;
    }

    // send a sync message to server for time sync
    int64_t t_1 = esp_timer_get_time();
    cJSON *sync_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(sync_msg, "type", "sync");
    cJSON_AddNumberToObject(sync_msg, "t_1", (double)t_1);
    char* sync_str = cJSON_PrintUnformatted(sync_msg);
    send(sock, sync_str, strlen(sync_str), 0);
    send(sock, "\n", 1, 0);
    cJSON_Delete(sync_msg);
    free(sync_str);

    // receive response from server
    while (1) {
        int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0) {
            ESP_LOGE(TAG, " recv failed: errno %d", errno);
        } else if (len == 0) {
            ESP_LOGW(TAG, "Server closed");
            break;
        } else {
            rx_buffer[len] = 0; // null-terminate
            ESP_LOGI(TAG, " Received from server: %s", rx_buffer);

            // Try to parse as JSON for sync/sync_resp only
            cJSON *root = cJSON_Parse(rx_buffer);
            if (root) {
                cJSON *type = cJSON_GetObjectItem(root, "type");
                if (type && strcmp(type->valuestring, "sync") == 0) {
                    // Perform time sync: send sync message with t_1
                    int64_t t_1 = esp_timer_get_time();
                    cJSON *sync_msg = cJSON_CreateObject();
                    cJSON_AddStringToObject(sync_msg, "type", "sync");
                    cJSON_AddNumberToObject(sync_msg, "t_1", (double)t_1);
                    char* sync_str = cJSON_PrintUnformatted(sync_msg);
                    send(sock, sync_str, strlen(sync_str), 0);
                    send(sock, "\n", 1, 0);
                    cJSON_Delete(sync_msg);
                    free(sync_str);
                    cJSON_Delete(root);
                    continue;
                } else if (type && strcmp(type->valuestring, "sync_resp") == 0) {
                    // Robust offset-based time sync logic
                    cJSON *t_1_item = cJSON_GetObjectItem(root, "t_1");
                    cJSON *t_2_item = cJSON_GetObjectItem(root, "t_2");
                    cJSON *t_3_item = cJSON_GetObjectItem(root, "t_3");
                    if (t_1_item && t_2_item && t_3_item &&
                        cJSON_IsNumber(t_1_item) && cJSON_IsNumber(t_2_item) && cJSON_IsNumber(t_3_item)) {
                        int64_t t_1 = (int64_t)(t_1_item->valuedouble);
                        int64_t t_2 = (int64_t)(t_2_item->valuedouble);
                        int64_t t_3 = (int64_t)(t_3_item->valuedouble);
                        int64_t t_4 = esp_timer_get_time();
                        int64_t new_offset = ((t_2 - t_1) + (t_3 - t_4)) / 2;
                        int64_t rtt = (t_4 - t_1) - (t_3 - t_2);
                        offset = new_offset;
                        ESP_LOGI(TAG, "SYNC_RESP: t_1=%lld t_2=%lld t_3=%lld t_4=%lld offset=%lld rtt=%lld", t_1, t_2, t_3, t_4, offset, rtt);
                    }
                    cJSON_Delete(root);
                    continue;
                }
                cJSON_Delete(root);
                // If not sync/sync_resp, fall through to string command parsing
            }

            // Handle string commands (play, pause, stop, parttest)
            char *cmd = rx_buffer;
            // Remove trailing newline if present
            size_t cmd_len = strlen(cmd);
            if (cmd_len > 0 && cmd[cmd_len - 1] == '\n') {
                cmd[cmd_len - 1] = '\0';
            }

            if (strncmp(cmd, "play", 4) == 0) {
                // play -ss<starttime> -end<endtime> -d<milisecond> -dd<delaydisplaytime>
                int starttime = 0, endtime = 0, d = 0, dd = 0;
                sscanf(cmd, "play -ss%d -end%d -d%d -dd%d", &starttime, &endtime, &d, &dd);
                ESP_LOGI(TAG, "PLAY: starttime=%d, endtime=%d, d=%d, dd=%d", starttime, endtime, d, dd);
                // TODO: implement play logic
            } else if (strcmp(cmd, "pause") == 0) {
                ESP_LOGI(TAG, "PAUSE command received");
                // TODO: implement pause logic
            } else if (strcmp(cmd, "stop") == 0) {
                ESP_LOGI(TAG, "STOP command received");
                // TODO: implement stop logic
            } else if (strncmp(cmd, "parttest", 8) == 0) {
                // parttest -rgb<rgb value> -c<channel>
                int rgb = 255, channel = 0;
                sscanf(cmd, "parttest -rgb%d -c%d", &rgb, &channel);
                ESP_LOGI(TAG, "PARTTEST: rgb=%d, channel=%d", rgb, channel);
                // TODO: implement parttest logic
            } else {
                ESP_LOGW(TAG, "Unsupported or malformed command: %s", cmd);
            }
        }
    }

    close(sock);
    vTaskDelete(NULL);
}

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Wi-Fi disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: %s", ip4addr_ntoa((ip4_addr_t*)&event->ip_info.ip));
        xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
    }
}
void app_main() {
    // initialization of wifi setting
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    // Register event handlers for Wi-Fi and IP events
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "LAPTOP-1L16PLG5 3420", // your wifi ssid
            .password = "66666666", // your wifi password
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Wi-Fi started");
}
