# remoteControl on ESP32
## Summary
This is a remote control project on ESP32, based on **TCP**. Mainly for the development in NTUEE lightdance
---
## File
### server.py
- the file you run on your laptop
- before you run the file, make sure to accomplish the following tasks and adjust the following parameter
- [ ] the IP address
- [ ] the port you use
- [ ] turn on the hotspot on your laptop
you can find them in 
```python=
server.bind()
```
---
### ESP32_remoreControl
- the file you run on your laptop
- before you run the file, make sure to accomplish the following tasks and adjust the following parameter
- [ ] the IP address
- [ ] the port you use
```C=
#define SERVER_IP   "your IP address"   // server IP
#define SERVER_PORT the port you use
```
- [ ] the ssid of your hotspot
- [ ] the password of your hotspot
```C=
wifi_config_t wifi_config = {
        .sta = {
            .ssid = "your ssid", // your wifi ssid
            .password = "your password", // your wifi password
        },
    };
```
---
## Goal and specific history
### one2one remoreControl
### one2one withjson
### one2multiple
