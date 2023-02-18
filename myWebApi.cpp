#include "myWebApi.h"
#include "debugTool.h"
#include <HTTPClient.h>

// Constructor ///////////////////////////////////////////////////////////////
MyWebApi::MyWebApi(){
  Serial.printf("myWebApi:Construct.\n");
  memset(server,0,sizeof(server));
  memset(echoUri,0,sizeof(echoUri));
  memset(postSsidUri,0,sizeof(postSsidUri));
}
// Destructor ////////////////////////////////////////////////////////////////
MyWebApi::~MyWebApi(){
  Serial.printf("myWebApi:Destruct.\n");
}
// Init //////////////////////////////////////////////////////////////////////
bool MyWebApi::init(char *serverAddress){
  Serial.printf("myWebApi::init: serverAddress=%s\n",serverAddress);

  // Create server address
  if(strlen(serverAddress)>(ADDRESS_SIZE-1)){
    Serial.printf("myWebApi::init : serverAddress is too long.\n");
    return false;
  }
  strcpy(server, serverAddress);

  // Create echo URI
  if(strlen(server)+strlen(ECHO)>(URI_SIZE-1)){
    Serial.printf("myWebApi::init : ECHO is too long.\n");
    return false;
  }
  strcat(echoUri,server);
  strcat(echoUri,ECHO);
  Serial.printf("myWebApi::init : echoUri=%s\n",echoUri);

  // Create postSsid URI
  if(strlen(server)+strlen(POSTSSID)>(URI_SIZE-1)){
    Serial.printf("myWebApi::init : POSTSSID is too long.\n");
    return false;
  }
  strcat(postSsidUri,server);
  strcat(postSsidUri,POSTSSID);
  Serial.printf("myWebApi::init : postSsidUri=%s\n",postSsidUri);

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
  http.begin(echoUri);
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
  http.begin(postSsidUri);
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

