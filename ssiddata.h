#ifndef SsidData_h
#define SsidData_h

#include <M5Core2.h>
#include <string.h>

#define ESSID_SIZE  32
#define BSSID_SIZE  18
#define DATETIME_SIZE 29

class SsidData {
  public:
    SsidData();
    ~SsidData();
    String  getJson();

    int     id;
    char  essid[ESSID_SIZE+1];
    char  bssid[BSSID_SIZE+1];
    long    rssi;
    int     frequency;
    double  latitude;
    double  longitude;
    char datetime[DATETIME_SIZE+1];
  private:
    String  _json;
};
#endif