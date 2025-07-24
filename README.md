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
### one2one remoteControl
In this version, server can send helloworld to the client(ESP32), and ESP32 will print it out through ESP-LOG.
As a reminder, these are all one2one.
### one2one with json
In this version, through the package __json__, we can send json file to others, but all the behaviors on ESP32 are not handled. Right now, they can only print a message on the log without doing anything.

(I haven't tested this version)
### one2multiple
In this version, through the package __threading__, we can send json file to others. server.py has been updated, Thankfully, I don't need to adjust the main.c a lot. 

(I haven't tested this version)
