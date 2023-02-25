#ifndef mywifi_h
#define mywifi_h

#include <M5Core2.h>
#include <string.h>
#include <WiFi.h>

#define ESSID_SIZE    32
#define PSK_SIZE      128
#define HOME_SSID_MAX 4
#define CONNECT_TIMEOUT 5000
class MyWifi {
  public:
    MyWifi();
    ~MyWifi();
    bool    connect(WiFiSTAClass wifi);
    void    disconnect(WiFiSTAClass wifi);
    bool    setSsid(unsigned int id, const char* ssidStr, const char* pskStr);
    String  getSsid(unsigned int id);
    String  getConnectedSsid();
  private:
    char    connectedSsid[ESSID_SIZE+1];
    char    essid[HOME_SSID_MAX][ESSID_SIZE+1];
    char    psk[HOME_SSID_MAX][PSK_SIZE+1];
};
#endif