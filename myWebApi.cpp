#include "myWebApi.h"
#include "debugTool.h"
#include <HTTPClient.h>

// Constructor ///////////////////////////////////////////////////////////////
MyWebApi::MyWebApi(){
  Serial.printf("myWebApi:Construct.\n");
  memset(_server,0,sizeof(_server));
  memset(_echoUri,0,sizeof(_echoUri));
  memset(_postSsidUri,0,sizeof(_postSsidUri));
}
// Destructor ////////////////////////////////////////////////////////////////
MyWebApi::~MyWebApi(){
  Serial.printf("myWebApi:Destruct.\n");
}
// Init //////////////////////////////////////////////////////////////////////
bool MyWebApi::init(const char *serverAddress){
  Serial.printf("myWebApi::init: serverAddress=%s\n",serverAddress);

  // Create server address
  if(strlen(serverAddress)>(ADDRESS_SIZE-1)){
    Serial.printf("myWebApi::init : serverAddress is too long.\n");
    return false;
  }
  strcpy(_server , serverAddress);

  // Create echo URI
  if(strlen(_server )+strlen(ECHO)>(URI_SIZE-1)){
    Serial.printf("myWebApi::init : ECHO is too long.\n");
    return false;
  }
  strcat(_echoUri,_server );
  strcat(_echoUri,ECHO);
  Serial.printf("myWebApi::init : _echoUri=%s\n",_echoUri);

  // Create postSsid URI
  if(strlen(_server )+strlen(POSTSSID)>(URI_SIZE-1)){
    Serial.printf("myWebApi::init : POSTSSID is too long.\n");
    return false;
  }
  strcat(_postSsidUri,_server );
  strcat(_postSsidUri,POSTSSID);
  Serial.printf("myWebApi::init : _postSsidUri=%s\n",_postSsidUri);

  return true;
}
// ECHO //////////////////////////////////////////////////////////////////////
bool MyWebApi::echo(String msg){
  bool  ret = false;
  int responseCode;
  int responseSize;
  String responseString;

  Serial.printf("myWebApi::echo: msg=%s\n",msg.c_str());

  HTTPClient http;
  http.begin(_echoUri);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  responseCode = http.POST(msg.c_str());

  Serial.printf("myWebApi::echo: responseCode=%d\n",responseCode);
  if (responseCode == 200) {
    responseSize = http.getSize();
    responseString = http.getString();
    Serial.println(responseString);
    if (responseString.compareTo(msg) == 0) {
      // post msg == responseString
      ret = true;
    }
  } else {
    ret = false;
  }
  http.end();
  return ret;
}

// POST SSID /////////////////////////////////////////////////////////////////
int MyWebApi::postSsid(String msg){
  int  ret = 0;
  int responseCode;
  int responseSize;
  String responseString;

  Serial.printf("myWebApi::postSsid: msg.length=%d\n",msg.length());

  HTTPClient http;
  http.begin(_postSsidUri);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  responseCode = http.POST(msg.c_str());

  Serial.printf("myWebApi::postSsid: responseCode=%d\n",responseCode);
  if (responseCode == 200) {
    responseSize = http.getSize();
    responseString = http.getString();

    Serial.printf("myWebApi::postSsid: responseString=%s\n",responseString.c_str());
    ret = atoi(responseString.c_str());
  } else {
    ret = -1;
  }
  http.end();
  return ret;
}

