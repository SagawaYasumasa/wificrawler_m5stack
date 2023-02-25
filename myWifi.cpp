/*
  Custom Wifi class
*/
#include  <limits.h>
#include "myWifi.h"

// Constructor //////////////////////////////////////////////////////////////
MyWifi::MyWifi(){
  int i;
  memset(connectedSsid,0, (ESSID_SIZE+1)*sizeof(char));
  for(i=0;i<HOME_SSID_MAX;i++){
    memset(essid[i],0,(ESSID_SIZE+1)*sizeof(char));
    memset(psk[i],0,(PSK_SIZE+1)*sizeof(char));
  }
}
// Destructor ////////////////////////////////////////////////////////////////
MyWifi::~MyWifi(){
}
// setSsid ///////////////////////////////////////////////////////////////////
bool MyWifi::setSsid(unsigned int id,const char* ssidStr, const char* pskStr){
  if(id>=HOME_SSID_MAX){
    Serial.printf("MyWifi::setSsid : id(%d) is too big.\n",id);
    return false;
  } 
  if(strlen(ssidStr)>ESSID_SIZE || strlen(pskStr)>PSK_SIZE){
    Serial.printf("MyWifi::setSsid : ssid or psk is too long.\n");
    return false;
  }
  strcpy(essid[id],ssidStr);
  strcpy(psk[id],pskStr);
  Serial.printf("MyWifi::setSsid : id=%d,ssid=%s, psk=%s\n",id,essid[id],psk[id]);
  return true;
}
// getSsid ///////////////////////////////////////////////////////////////////
String MyWifi::getSsid(unsigned int id){
  String ret="";
  if(id>=HOME_SSID_MAX){
    Serial.printf("MyWifi::getSsid : id(%d) is too big.\n",id);
  } else {
    ret = essid[id];
  }
  return ret;   
}
// getConnectedSsid ///////////////////////////////////////////////////////////////////
String MyWifi::getConnectedSsid(){
  String ret;
  ret = connectedSsid;
  return ret;   
}

// connect ///////////////////////////////////////////////////////////////////
bool MyWifi::connect(WiFiSTAClass wifi){
  unsigned int  id;
  long  t;  
  int ret;

  for(id=0;id<HOME_SSID_MAX;id++){
    if(strlen(essid[id])==0){continue;}
    Serial.printf("\nMyWifi::connect : id=%d, essid=%s, psk=%s\n",id,essid[id],psk[id]);
    t = millis();
    wifi.begin(essid[id], psk[id]);
    while( millis() < (t + CONNECT_TIMEOUT)){
      delay(100);
      ret = wifi.status();
      Serial.printf("%d",ret);
      if(ret == WL_CONNECTED){
        Serial.printf("\nMyWifi::connect : success.\n");
        strcpy(connectedSsid,essid[id]);
        return true;  // Exit this method
      }
    }
  }
  Serial.printf("\nMyWifi::connect : failed.\n");
  return false;
}
// disconnect ////////////////////////////////////////////////////////////////
void MyWifi::disconnect(WiFiSTAClass wifi){
  wifi.disconnect();
  memset(connectedSsid,0, (ESSID_SIZE+1)*sizeof(char));
  delay(100);
  Serial.printf("MyWifi::disconnect : success.\n");
}

