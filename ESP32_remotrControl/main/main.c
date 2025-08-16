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
#include "esp_timer.h"

#define SERVER_IP   "192.168.137.155"
#define SERVER_PORT 5000

static const char *TAG = "TCP_CLIENT";
static int64_t offset = 0;

static void send_sync_with_t1(int sock) {
    int64_t t_1 = esp_timer_get_time();
    char line[128];
    snprintf(line, sizeof(line), "sync -t_1 %lld\n", (long long)t_1);
    send(sock, line, strlen(line), 0);
}

static void process_line(int sock, char *line) {
    // line is null terminated without trailing \n
    if (strncmp(line, "syncresp", 8) == 0) {
        // Expected: syncresp -t1 <t1> -t2 <t2> -t3 <t3>
        int64_t t1 = 0, t2 = 0, t3 = 0;
        char *p = line + 8;
        while (*p) {
            while (*p == ' ') p++;
            if (strncmp(p, "-t1", 3) == 0) {
                p += 3; while (*p == ' ') p++; if (*p) { t1 = strtoll(p, &p, 10); }
            } else if (strncmp(p, "-t2", 3) == 0) {
                p += 3; while (*p == ' ') p++; if (*p) { t2 = strtoll(p, &p, 10); }
            } else if (strncmp(p, "-t3", 3) == 0) {
                p += 3; while (*p == ' ') p++; if (*p) { t3 = strtoll(p, &p, 10); }
            } else {
                while (*p && *p != ' ') p++; // skip unknown token
            }
        }
        int64_t t4 = esp_timer_get_time();
        int64_t new_offset = ((t2 - t1) + (t3 - t4)) / 2;
        int64_t rtt = (t4 - t1) - (t3 - t2);
        offset = new_offset;
        ESP_LOGI(TAG, "SYNCRESP: t1=%lld t2=%lld t3=%lld t4=%lld offset=%lld rtt=%lld",
                 (long long)t1,(long long)t2,(long long)t3,(long long)t4,(long long)offset,(long long)rtt);
        return;
    }
    if (strncmp(line, "sync", 4) == 0) {
        // Server asked sync without times: respond with sync -t_1 <t1>
        send_sync_with_t1(sock);
        return;
    }
    if (strncmp(line, "play", 4) == 0) {
        int starttime=0,endtime=0,d=0,dd=0;
        sscanf(line, "play -ss%d -end%d -d%d -dd%d", &starttime,&endtime,&d,&dd);
        ESP_LOGI(TAG, "PLAY: ss=%d end=%d d=%d dd=%d", starttime,endtime,d,dd);
        return;
    }
    if (strcmp(line, "pause") == 0) { ESP_LOGI(TAG, "PAUSE"); return; }
    if (strcmp(line, "stop") == 0) { ESP_LOGI(TAG, "STOP"); return; }
    if (strncmp(line, "parttest", 8) == 0) {
        int channel=0; int r=255,g=255,b=255;
        char local[256]; strncpy(local,line,sizeof(local)-1); local[sizeof(local)-1]='\0';
        char *tok=strtok(local," "); // parttest
        tok=strtok(NULL," ");
        while(tok){
            if(strcmp(tok,"-c")==0){ char *v=strtok(NULL," "); if(v) channel=atoi(v); }
            else if(strcmp(tok,"-rgb")==0){ char *sr=strtok(NULL," "); char *sg=strtok(NULL," "); char *sb=strtok(NULL," "); if(sr&&sg&&sb){ r=atoi(sr); g=atoi(sg); b=atoi(sb);} }
            tok=strtok(NULL," ");
        }
        ESP_LOGI(TAG, "PARTTEST: c=%d rgb=(%d,%d,%d)",channel,r,g,b);
        return;
    }
    ESP_LOGW(TAG, "Unknown line: %s", line);
}

void tcp_client_task(void *pvParameters) {
    char rx_buffer[1024];
    struct sockaddr_in dest_addr;
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (sock < 0) { ESP_LOGE(TAG, "socket fail errno %d", errno); vTaskDelete(NULL); return; }
    dest_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(SERVER_PORT);
    ESP_LOGI(TAG, "Connecting to %s:%d", SERVER_IP, SERVER_PORT);
    if (connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) != 0) { ESP_LOGE(TAG, "connect fail errno %d", errno); close(sock); vTaskDelete(NULL); return; }

    // initial sync request
    send_sync_with_t1(sock);

    char accum[1024]; size_t acc_len = 0;
    while (1) {
        int len = recv(sock, rx_buffer, sizeof(rx_buffer)-1, 0);
        if (len < 0) { ESP_LOGE(TAG, "recv errno %d", errno); break; }
        if (len == 0) { ESP_LOGW(TAG, "server closed"); break; }
        rx_buffer[len] = '\0';
        for(int i=0;i<len;i++){
            char ch = rx_buffer[i];
            if(ch=='\n'){
                accum[acc_len]='\0';
                if(acc_len>0) process_line(sock, accum);
                acc_len=0;
            } else if(acc_len < sizeof(accum)-1){
                accum[acc_len++]=ch;
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
