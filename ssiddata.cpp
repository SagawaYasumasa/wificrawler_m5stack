#include "ssiddata.h"
#include "debugTool.h"
#include <string.h>

// Constructor //////////////////////////////////////////////////////////////
SsidData::SsidData(){
  id = 0;
  memset(essid,0x00,sizeof(essid));
  memset(bssid,0x00,sizeof(bssid));
  rssi = 0;
  frequency = 0;
  latitude = 0.0;
  longitude = 0.0;
  memset(datetime,0x00,sizeof(datetime));
  _json = "";
}
// Destructor ///////////////////////////////////////////////////////////////
SsidData::~SsidData(){

}

String SsidData::getJson(){
  char tmp[256]={0};
  char tmpLatitude[11+1]="";
  char tmpLongitude[11+1]="";

  dtostrf(latitude,11,6,tmpLatitude);
  dtostrf(longitude,11,6,tmpLongitude);
  sprintf(tmp,"{\"ID\":\"%d\",\"ESSID\":\"%s\",\"BSSID\":\"%s\",\"RSSI\":\"%ld\",\"FREQUENCY\":\"%d\",\"LATITUDE\":\"%s\",\"LONGITUDE\":\"%s\",\"DATETIME\":\"%s\"}"
          ,id,essid,bssid,rssi,frequency,tmpLatitude,tmpLongitude,datetime);
  _json = tmp;
  return _json;
}
