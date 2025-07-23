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

#define SERVER_IP   "192.168.1.83"   // server IP
#define SERVER_PORT 4000

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

    // send a string server
    char *message = "Hi! This is ESP32.\n";
    send(sock, message, strlen(message), 0);

    // receive response from server
    int len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
    if (len < 0) {
        ESP_LOGE(TAG, " recv failed: errno %d", errno);
    } else {
        rx_buffer[len] = 0; // null-terminate
        ESP_LOGI(TAG, " Received from server: %s", rx_buffer);
    }

    close(sock);
    vTaskDelete(NULL);
}

void app_main() {
    // initialization of wifi setting
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

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

    ESP_ERROR_CHECK(esp_wifi_connect());

    // after setting wifi, execute the tcp_client_task(hello world)
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);
}
