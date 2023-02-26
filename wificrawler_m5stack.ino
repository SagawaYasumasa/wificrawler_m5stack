#define MAJOR_VERSION 1
#define MINOR_VERSION 5

#include "private.h"
#define CONFIG_FILE_NAME  "/system.ini"
#define DATA_FILE_NAME "/json.txt"

#define RSSI_VALID_VALUE  -75 

#include <M5Core2.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <TinyGPSPlus.h>

#include "inifile.h"
#include "ssiddata.h"
#include "tty.h"
#include "myWebApi.h"
#include "myWifi.h"
#include "debugTool.h"

// timer
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;
// timer variables
volatile unsigned long gTimerCounter = 0;
// Parameters
size_t  paramFileSizeMax=0;

static const uint32_t GPSBaud = 9600;
static const int Font1Height = 8;  // Font=1, Adafruit 8 pixels ascii font
static const long WifiScanInterval = 3000; // 3000 msec
static const long AsyncScanTimeout = 10 * 1000; // Wi-Fi scan Timeout(msec)
static const long PassiveScanTimeout = 300; // passive scan timeout at 1channel(msec)

//Vibration
AXP192  power;
//Creat The TinyGPS++ object.
TinyGPSPlus gps;
// The serial connection to the GPS device.
HardwareSerial ss(2);

Inifile   inifile;
SsidData  ssidData;
Tty       upperTty;
Tty       lowerTty;
MyWifi    myWifi;
// global variables
volatile long LastScanMillis;
/******************************************************************************/
//  Interrupt Service Routine
/******************************************************************************/
void IRAM_ATTR onTimer() {
  portENTER_CRITICAL_ISR(&timerMux);
  gTimerCounter++;

  portEXIT_CRITICAL_ISR(&timerMux);
}

void setup() {
  bool  ret;
  char  tmpStr[256]={0};
  String  valueString;
  String  ssidString;
  String  pskString;  
  M5.begin();                             //Init M5Stack.
  SD.begin();
  ss.begin(GPSBaud, SERIAL_8N1, 33, 32);  //It requires the use of SoftwareSerial, and assumes that you have a 4800-baud serial GPS device hooked up on pins 4(rx) and 3(tx).

  // Init global variables
  LastScanMillis = 0;
  // Init Timer
  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000 * 10, true);  // 10msec interval
  timerAlarmEnable(timer);

  // Init TTYs
  upperTty.init(0,0,M5.Lcd.width(),64,TFT_WHITE,TFT_BLACK);
  lowerTty.init(0, 64, M5.Lcd.width(), M5.Lcd.height()-64, TFT_WHITE, TFT_BLACK);

  // Init Paramaters
  // "FILE_SIZE_MAX"
  ret = inifile.getValue(CONFIG_FILE_NAME,"FILE_SIZE_MAX",valueString);
  if(!ret){
    lowerTty.putString("No paramater:FILE_SIZE_MAX\n");
    while(true){};  // System halt
  }  
  paramFileSizeMax = (size_t)atoi(valueString.c_str());
  sprintf(tmpStr,"paramFileSizeMax=%d\n",paramFileSizeMax);
  lowerTty.putString(tmpStr);
  // "HOME_SSID" and "HOME_SSID_PASS"
  unsigned int i;
  for(i=0;i<HOME_SSID_MAX;i++){
    sprintf(tmpStr,"HOME_SSID_%d",i);
    inifile.getValue(CONFIG_FILE_NAME,tmpStr,ssidString);
    sprintf(tmpStr,"HOME_SSID_PASS_%d",i);
    inifile.getValue(CONFIG_FILE_NAME,tmpStr,pskString);
    myWifi.setSsid(i,ssidString.c_str(),pskString.c_str());
  }

  WiFi.mode(WIFI_STA);  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.disconnect();    //Turn off all wifi connections.
  delay(100);           //100 ms delay.

  sprintf(tmpStr,"Wi-Fi Crawler for M5Stack. version %d.%02d", MAJOR_VERSION, MINOR_VERSION);
  upperTty.writeLine(0,tmpStr);

  while(gps.charsProcessed()<10){
    if(millis()>10000){
      upperTty.writeLine(2,"No GPS data received: check wiring");
      smartDelay(10000);
    }
    smartDelay(500);
  }  
    if (millis() > 10000 && gps.charsProcessed() < 10) {
  }
  while (!gps.satellites.isValid()) { checkUserAction(); smartDelay(100); }
  upperTty.writeLine(2,"GSP:Satellites OK.");
  while (!gps.hdop.isValid()) { checkUserAction(); smartDelay(100); }
  upperTty.writeLine(3,"GPS:HDOP OK.");
  while (!gps.date.isValid()) { checkUserAction(); smartDelay(100); }
  while (!gps.time.isValid()) { checkUserAction(); smartDelay(100); }
  setRtc(gps.date, gps.time);
  upperTty.writeLine(4,"GPS:DATE,TIME OK.");
  while (!gps.location.isValid()) { checkUserAction(); smartDelay(100); }
  upperTty.writeLine(5,"GPS:LOCATION OK.");
  smartDelay(100);
}

void loop() {
  long        asyncScanStart = 0; // millis();
  int         scanResult;

  int numberOfWifi = 0;
  double latitude = 0.0;
  double longitude = 0.0;
//  char  datetime[DATETIME_SIZE + 1] = { 0 };
  char  tempStr[256]={0};
  int   markX, markY;
  int   markChar = ' ';     
  RTC_TimeTypeDef   rtcTime;
  RTC_DateTypeDef   rtcDate;

  markX = upperTty.getColumnSize()-1;       // TTY RIGHT
  markY = 0;                                // TTY TOP

  // Check GPS status 
  while (!gps.location.isValid()) {
    upperTty.writeChar(markX, markY, '?');
    smartDelay(50); 
  }
  upperTty.writeChar(markX, markY, ' ');
  latitude = gps.location.lat();
  longitude = gps.location.lng();

  // Display GPS information
  printGpsSatellites(gps.satellites.value(), gps.satellites.isValid());
  printGpsHdop(gps.hdop.value(), gps.hdop.isValid());
  printGpsDateTime(gps.date, gps.time);
  printGpsLocation(latitude, longitude, true);

  // check button ////////////////////////////////////////////////////////////
  checkUserAction();
  // Scanning Wi-Fi //////////////////////////////////////////////////////////
  if( isScanEnable() && millis()>(LastScanMillis+WifiScanInterval)){
    asyncScanStart = millis();    // set Scan Start Time  
    WiFi.scanNetworks(true, false, true, PassiveScanTimeout);
    do {
      markChar = (markChar == '*') ? '+' : '*';   // alternate '*' <-> '+'
      upperTty.writeChar(markX, markY, markChar);
      smartDelay(100);
      scanResult = WiFi.scanComplete();
      if(scanResult != WIFI_SCAN_RUNNING){
        break;
      }
      // check buttun //////////////////////////////////////////////////////////
      checkUserAction();
    }while(millis() < (asyncScanStart+AsyncScanTimeout));

    // after Wi-Fi scan ///////////////////////////////////////////////////////
    upperTty.writeChar(markX, markY, ' ');
    LastScanMillis = millis();
    numberOfWifi = scanResult;
    if (numberOfWifi == 0 || numberOfWifi == WIFI_SCAN_FAILED) {
      Serial.printf("scanResult=%d\n",scanResult);
      // no networks found
      lowerTty.putString("no networks found.\n");
    } else {
      M5.Rtc.GetDate(&rtcDate);
      M5.Rtc.GetTime(&rtcTime);   
      // Wi-Fi Scan success ////////////////////////////////////////////////////
      if (gps.location.isValid()) {  // re-check location
        latitude = gps.location.lat();
        longitude = gps.location.lng();
      }
      String json = "";
      for (int i = 0; i < numberOfWifi; ++i) {  // Print SSID and RSSI for each network found.
        if(WiFi.RSSI(i)>RSSI_VALID_VALUE){
          ssidData.id = i;
          WiFi.SSID(i).getBytes((unsigned char *)ssidData.essid, sizeof(ssidData.essid));
          convertMacAddr(WiFi.BSSID(i), ssidData.bssid);
          ssidData.rssi = WiFi.RSSI(i);
          ssidData.frequency = 0;
          ssidData.latitude = latitude;
          ssidData.longitude = longitude;
          sprintf(ssidData.datetime,"%04d-%02d-%02d %02d:%02d:%02d",
              gps.date.year(),rtcDate.Month,rtcDate.Date,rtcTime.Hours,rtcTime.Minutes,rtcTime.Seconds);
          json = json + ssidData.getJson() + ",\n";
          sprintf(tempStr,"%02d:%s %s RSSI=%ld\n",ssidData.id, ssidData.essid, ssidData.bssid, ssidData.rssi);
          lowerTty.putString(tempStr);
        }
      }
      writeRecord(json.c_str());
    }
  }
  smartDelay(20);
}
unsigned int checkUserAction(){
//static unsigned int checkUserAction(void){
  char  tempStr[256]={0};  
  unsigned int result = 0x00000000;

  M5.update();  //Check the status of the key.
  // Button A /////////////////////////////////////////////////////////////
  if (M5.BtnA.isPressed()) {  //If button A is pressed.
    MyWebApi  heatmapApi;
    String  echoMsg = "[{\"CLIENT\":\"ARDUINO\"}]";
    String  postMsg = "";
    String  server ="";
    int result; 
    int recordCount;   

    lowerTty.putString("Button A pressed.\n");
    vibration(200);

   // "SERVER"
    inifile.getValue(CONFIG_FILE_NAME,"SERVER",server);
    heatmapApi.init(server.c_str());
    
    lowerTty.putString("connecting Wi-Fi...\n");
    if(myWifi.connect(WiFi)){
      sprintf(tempStr,"Connect to %s\n",myWifi.getConnectedSsid().c_str());
      lowerTty.putString(tempStr);
      if(heatmapApi.echo(echoMsg)){
        // server avaliable
        lowerTty.putString("WebApi ECHO success.\n");
        recordCount = createPostMsg(postMsg);
        switch (recordCount){
          case  -1:
            lowerTty.putString((char *)"failed to memory allocation.(postMsg)\n");
            break;
          case  0: 
            lowerTty.putString((char *)"No SSID record.\n");
            break;
          default:
            // POST SSID data
            sprintf(tempStr,"Post     %d record(s).\n", recordCount);    
            lowerTty.putString(tempStr);
            result = heatmapApi.postSsid(postMsg);
            sprintf(tempStr,"Response %d record(s).\n", result);    
            lowerTty.putString(tempStr);
            if(recordCount == result){
              // Post successful
              lowerTty.putString("Post SSID record successful.\n");
              lowerTty.putString("Delete File.\n");
              deleteFile();
              smartDelay(2000);
            }
        }
      } else {
        lowerTty.putString("WebApi ECHO failed.\n");
      }
    }
    myWifi.disconnect(WiFi);   
    result = result | 0x00000001;
  }
  if (M5.BtnB.isPressed()) {  //If button B is pressed.
    lowerTty.putString("Button B pressed.\n");
    vibration(200);
    displayParameters();

    result = result | 0x00000002;
  }
  if (M5.BtnC.isPressed()) {  //If button C is pressed.
    lowerTty.putString("Button C pressed.\n");
    vibration(200);
    displayHelp();
/*
    lowerTty.putString("Dump SD " DATA_FILE_NAME "\n");
    dumpRecord();
    smartDelay(2000);
*/
    result = result | 0x00000004;
  }
  return result;    
}

// This custom version of delay() ensures that GPS objects work properly.
static void smartDelay(unsigned long ms) {
  unsigned long start = millis();
  do {
    while (ss.available()) {
      gps.encode(ss.read());
    }
  } while (millis() - start < ms);
}
// Print number of acquired satellites
static void printGpsSatellites(int numberOfSatellites, bool valid) {
  const int row = 2;
  char  tempStr[256]={0};  
  if (valid) {
    sprintf(tempStr,"Number of acquired satellite(s) : %d", numberOfSatellites);
  } else {
    sprintf(tempStr,"Invalid number of acquired satellite");
  }
  upperTty.writeLine(row, tempStr);
}
// Print horizontal dilution of Precision(HDOP)
static void printGpsHdop(int hdop, bool valid) {
  const int row = 3;
  char  tempStr[256]={0};  
  if (valid) {
    sprintf(tempStr,"Horizontal dilution of precision(HDOP) : %d", hdop);    
  } else {
    sprintf(tempStr,"Invalid HDOP");
  }
  upperTty.writeLine(row, tempStr);
}
// Print Date and Time(UTC+0000)
static void printGpsDateTime(TinyGPSDate &d, TinyGPSTime &t) {
  const int row = 4;
  char  tempStr[256]={0};  
  char tmpDate[14 + 1]={0};  // "YYYY/MM/DD"
  char tmpTime[14 + 1]={0};   //  "hh:mm:ss"

  memset(tmpDate, 0x00, sizeof(tmpDate));
  memset(tmpTime, 0x00, sizeof(tmpTime));
  if (d.isValid()) {
    sprintf(tmpDate, "%04d/%02d/%02d", d.year(), d.month(), d.day());
  } else {
    sprintf(tmpDate, "****/**/**");
  }
  if (t.isValid()) {
    sprintf(tmpTime, "%02d:%02d:%02d", t.hour(), t.minute(), t.second());
  } else {
    sprintf(tmpTime, "**:**:**");
  }
  sprintf(tempStr,"%s %s (UTC+0000)", tmpDate, tmpTime);
  upperTty.writeLine(row, tempStr);
  smartDelay(0);
}
// Print Latitude and Longitude
static void printGpsLocation(double latitude, double longitude, bool valid) {
  const int row = 5;
  char  tempStr[256]={0};  
  char tmpLat[11 + 1];
  char tmpLng[11 + 1];

  memset(tmpLat, 0x00, sizeof(tmpLat));
  memset(tmpLng, 0x00, sizeof(tmpLng));
  if (valid) {
    dtostrf(latitude, 11, 6, tmpLat);
    dtostrf(longitude, 11, 6, tmpLng);
  } else {
    sprintf(tmpLat, " ***.******");
    sprintf(tmpLng, " ***.******");
  }
  sprintf(tempStr,"Latitude :%s", tmpLat);
  upperTty.writeLine(row, tempStr);
  sprintf(tempStr,"Longitude:%s", tmpLng);
  upperTty.writeLine(row+1, tempStr);
}

bool writeRecord(const char *record) {
  File file;
  file = SD.open(DATA_FILE_NAME, FILE_APPEND);
  if(file){
    file.print(record);
    file.close();
  }
  return (file != false);
}
int createPostMsg(String& postMsg ){
  char *buf;
  int ptr = 0;
  int recordCount = 0;
  File file;

  Serial.printf("function:createPostMsg\n");
  file = SD.open(DATA_FILE_NAME, FILE_READ);
  Serial.printf("function:createPostMsg, available=%d\n", file.available());

  buf = (char *)malloc(file.size() + 2);  // top='[' ,tail=0x00
  if(buf == NULL){
    lowerTty.putString("createPostMsg: failed to memory allocation.\n");
    file.close();
    return -1;    
  }
  buf[ptr++] = '[';                       // start json
  while (0 < file.available()) {
    buf[ptr] = file.read();
    if(buf[ptr]==0x0a){ recordCount++; }
    ptr++;
  }
  buf[ptr] = 0x00;  // tail is null
  file.close();

  postMsg = buf;
  ptr = postMsg.lastIndexOf(',');
  postMsg.setCharAt(ptr, ']');  // replace last ',' to ']'. finish json

  free(buf);
  return recordCount;
}
bool isScanEnable(){
  File file;
  size_t  size = 0;
  
  file = SD.open(DATA_FILE_NAME, FILE_READ);
  size = file.available();
  file.close();
  if(size > paramFileSizeMax){
    lowerTty.putString("datafile size too large.\n");
    
    while(checkUserAction() == 0){
      smartDelay(100);      
    }
    return false;
  } else {
    return true;
  }
}
int dumpRecord() {
  const char *fileName = DATA_FILE_NAME;
  char *buf;
  int ptr = 0;
  String jsonString;
  File file;

  Serial.printf("function:dumpRecord\n");
  file = SD.open(fileName, FILE_READ);
  Serial.printf("function:dumpRecord, available=%d\n", file.available());

  buf = (char *)malloc(file.size() + 2);  // top='[' ,tail=0x00
  if(buf == NULL){
    lowerTty.putString((char *)"dumpRecord: failed to memory allocation.\n");
    file.close();
    return 0;    
  }
  buf[ptr++] = '[';                       // start json
  while (0 < file.available()) {
    buf[ptr++] = file.read();
  }
  buf[ptr] = 0x00;  // null
  file.close();
lowerTty.putString(buf);

  jsonString = buf;
  ptr = jsonString.lastIndexOf(',');
  jsonString.setCharAt(ptr, ']');  // replace last ',' to ']'. finish json

  Serial.println(jsonString);
  free(buf);
  Serial.printf("function:dumpRecord, return(%d)\n", jsonString.length());

  return jsonString.length();
}
bool deleteFile() {
  bool ret = false;
  const char *fileName = DATA_FILE_NAME;

  Serial.printf("function:DeleteFile\n");
  ret = SD.remove(fileName);
  Serial.printf("function:DeleteFile, return(%d)\n", ret);
  return ret;
}
// convert Mac Address( binary to charactor string) //////////////////////////
void convertMacAddr(uint8_t *bin, char *str) {
  sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X", bin[0], bin[1], bin[2], bin[3], bin[4], bin[5]);
}
// set RTC ///////////////////////////////////////////////////////////////////
void setRtc(TinyGPSDate &d, TinyGPSTime &t){
  RTC_TimeTypeDef   rtcTime;
  RTC_DateTypeDef   rtcDate;

  if(d.isValid() && t.isValid()){
    rtcDate.Year = d.year();  // RTC YEAR is not available ( fiexed 2000 )
    rtcDate.Month = d.month();
    rtcDate.Date = d.day();
    rtcTime.Hours = t.hour();
    rtcTime.Minutes = t.minute();
    rtcTime.Seconds = t.second();
    M5.Rtc.SetDate(&rtcDate);
    M5.Rtc.SetTime(&rtcTime);
  }
}
// Vibration /////////////////////////////////////////////////////////////////
void vibration(int msec){
  power.SetLDOEnable(3,true);
  delay(msec);
  power.SetLDOEnable(3,false);
}
// Display Parameters ////////////////////////////////////////////////////////
void displayParameters(){
  char  tmpStr[256]= {0};
  String  value;  
  long  msec;
  int   i;
  lowerTty.clear();  

  lowerTty.putString("System Parameters\n");
  // FILE_SIZE_MAX
  sprintf(tmpStr,"FILE_SIZE_MAX=%d\n",paramFileSizeMax);
  lowerTty.putString(tmpStr);
  // HOME_SSID
  for(i=0;i<HOME_SSID_MAX;i++){
    sprintf(tmpStr,"HOME_SSID_%d=%s\n",i,myWifi.getSsid(i).c_str());
    lowerTty.putString(tmpStr);
  }
  // SERVER
  inifile.getValue(CONFIG_FILE_NAME,"SERVER",value);
  sprintf(tmpStr,"SERVER=%s\n",value.c_str());
  lowerTty.putString(tmpStr);

  lowerTty.putString("\n");
  lowerTty.putString("Press BUTTON B to exit.\n");
  delay(1000);

  msec = millis();      
  while(millis() < msec + (30 * 1000)){
    M5.update();  //Check the status of the key.
    if (M5.BtnB.isPressed()) {
      vibration(200);
      break;
    }
    delay(100);
  }
  lowerTty.clear();
}
// Display help //////////////////////////////////////////////////////////////
void displayHelp(){
  long  msec;
  lowerTty.clear();  

  lowerTty.putString("Wi-Fi Crawler Help\n");
  lowerTty.putString("BUTTON A ... Post record to Server.\n");
  lowerTty.putString("BUTTON B ... Display system parameters.\n");
  lowerTty.putString("BUTTON C ... Display this contents.\n");
  lowerTty.putString("\n");
  lowerTty.putString("Press BUTTON C to exit.\n");
  delay(1000);

  msec = millis();      
  while(millis() < msec + (30 * 1000)){
    M5.update();  //Check the status of the key.
    if (M5.BtnC.isPressed()) {
      vibration(200);
      break;
    }
    delay(100);
  }
  lowerTty.clear();
}

