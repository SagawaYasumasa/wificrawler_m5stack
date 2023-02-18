#define MAJOR_VERSION 1
#define MINOR_VERSION 0

#include "private.h"
#define DATA_FILE_NAME "/json.txt"

#include <M5Core2.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGPSPlus.h>

#include "ssiddata.h"
//#include "ssidDisplay.h"
#include "tty.h"
#include "myWebApi.h"
#include "debugTool.h"

// timer
hw_timer_t *timer = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

// timer variables
volatile unsigned long gTimerCounter = 0;
//
static const uint32_t GPSBaud = 9600;
static const int Font1Height = 8;  // Font=1, Adafruit 8 pixels ascii font
static const long WifiScanInterval = 3000; // 3000 msec
static const long AsyncScanTimeout = 10 * 1000; // Wi-Fi scan Timeout(msec)
static const long PassiveScanTimeout = 300; // passive scan timeout at 1channel(msec)


//Vibration
// AXP192  power;
//Creat The TinyGPS++ object.
TinyGPSPlus gps;
// The serial connection to the GPS device.
HardwareSerial ss(2);

SsidData ssidData;
//SsidDisplay ssidDisplay;
Tty upperTty;
Tty lowerTty;
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
  char  tmpStr[256]={0};
  M5.begin();                             //Init M5Stack.
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

  WiFi.mode(WIFI_STA);  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.disconnect();    //Turn off all wifi connections.
  delay(100);           //100 ms delay.

  sprintf(tmpStr,"Wi-Fi Clawer for M5Stack. version %d.%02d", MAJOR_VERSION, MINOR_VERSION);
  upperTty.writeLine(0,tmpStr);
  sprintf(tmpStr,"User name:%s", MYNAME);
  upperTty.writeLine(1,tmpStr);

  while(gps.charsProcessed()<10){
    if(millis()>10000){
      upperTty.writeLine(2,"No GPS data received: check wiring");
      smartDelay(10000);
    }
    smartDelay(500);
  }  
    if (millis() > 10000 && gps.charsProcessed() < 10) {
  }
  while (!gps.satellites.isValid()) { smartDelay(500); }
  upperTty.writeLine(2,"GSP:Satellites OK.");
  while (!gps.hdop.isValid()) { smartDelay(500); }
  upperTty.writeLine(3,"GPS:HDOP OK.");
  while (!gps.date.isValid()) { smartDelay(500); }
  while (!gps.time.isValid()) { smartDelay(500); }
  upperTty.writeLine(4,"GPS:DATE,TIME OK.");
  while (!gps.location.isValid()) { smartDelay(500); }
  upperTty.writeLine(5,"GPS:LOCATION OK.");
  smartDelay(100);
}

void loop() {
  long        asyncScanStart = 0; // millis();
  int         scanResult;
  unsigned int userAction;

  int numberOfWifi = 0;
  double latitude = 0.0;
  double longitude = 0.0;
  char  datetime[DATETIME_SIZE + 1] = { 0 };
  char  tempStr[256]={0};
  int   markX, markY;
  int   markChar = ' ';     

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
  userAction = checkUserAction();

  // Scanning Wi-Fi //////////////////////////////////////////////////////////
  if( millis()> (LastScanMillis+WifiScanInterval)){
    asyncScanStart = millis();    // set Scan Start Time  
    WiFi.scanNetworks(true, false, false, PassiveScanTimeout);
    do {
      markChar = (markChar == '*') ? '+' : '*';   // alternate '*' <-> '+'
      upperTty.writeChar(markX, markY, markChar);
      smartDelay(100);
      scanResult = WiFi.scanComplete();
      if(scanResult != WIFI_SCAN_RUNNING){
        break;
      }
      // check buttun //////////////////////////////////////////////////////////
      userAction = checkUserAction();
    }while(millis() < (asyncScanStart+AsyncScanTimeout));

    // after Wi-Fi scan ///////////////////////////////////////////////////////
    upperTty.writeChar(markX, markY, ' ');
    LastScanMillis = millis();
    numberOfWifi = scanResult;
    if (numberOfWifi == 0 || numberOfWifi == WIFI_SCAN_FAILED) {
      Serial.printf("scanResult=%d\n",scanResult);
      // no networks found
      lowerTty.putString((char *)"no networks found.\n");
/*
      if(numberOfWifi == WIFI_SCAN_FAILED){
        smartDelay(3000);
      }
*/
    } else {
      // Wi-Fi Scan success ////////////////////////////////////////////////////
      if (gps.location.isValid()) {  // re-check location
        latitude = gps.location.lat();
        longitude = gps.location.lng();
      }
      String json = "";
      for (int i = 0; i < numberOfWifi; ++i) {  // Print SSID and RSSI for each network found.
        ssidData.id = i;
        WiFi.SSID(i).getBytes((unsigned char *)ssidData.essid, sizeof(ssidData.essid));
        convertMacAddr(WiFi.BSSID(i), ssidData.bssid);
        ssidData.rssi = WiFi.RSSI(i);
        ssidData.frequency = 0;
        ssidData.latitude = latitude;
        ssidData.longitude = longitude;
        strcpy(ssidData.datetime, "2023-01-01 00:00:00");
        json = json + ssidData.getJson() + ",\n";
        sprintf(tempStr,"%02d:%s %s RSSI=%d\n",ssidData.id, ssidData.essid, ssidData.bssid, ssidData.rssi);
        lowerTty.putString(tempStr);
      }
      writeRecord((char *)json.c_str());
    }
  }
    smartDelay(20);
}
/*
//  SsidData ssidData = SsidData();

  // Buton A /////////////////////////////////////////////////////////////
  if (M5.BtnA.isPressed()) {  //If button A is pressed.
    M5.Lcd.clear();           //Clear the screen.
    M5.Lcd.println("scan start");
    // Init Wi-Fi scan 
//   int n= WiFi.scanNetworks(false, false, false, 100);  //return the number of networks found. Active Scan
//    int n = WiFi.scanNetworks(false, false, true,150);  //return the number of networks found. Passive Scan
    WiFi.scanNetworks(true, false, true,150);     // async,passive mode

    int numberOfWifi = WIFI_SCAN_RUNNING;
    unsigned long timeout = 0;    
    // Init GPS scan
    int gpsStatus = 0;    // 0=scanning gps
    timeout = millis() + 5000; // 5000 milliseconds
    while( millis() < timeout ){
      // Scanning Wi-Fi
      if(numberOfWifi == WIFI_SCAN_RUNNING){
        numberOfWifi = WiFi.scanComplete();
      } 
      if((numberOfWifi != WIFI_SCAN_RUNNING) && (gpsStatus !=0)){
        // WiFi & GPS scan complete, or scan failed
        break;
      }        
      M5.Lcd.printf(".");
      delay(10);
    }
    Serial.printf("\nScan result numberOfWifi=%d,gpsStatus=%d\n",numberOfWifi,gpsStatus);

    if ((numberOfWifi == WIFI_SCAN_RUNNING) ||(numberOfWifi == WIFI_SCAN_FAILED) ||(numberOfWifi == 0)) {  //If no network is found.
      M5.Lcd.println("\nno networks found");
    } else {  //If have network is found.
      String json="";
      M5.Lcd.printf("\nnetworks found:%d\n\n", numberOfWifi);
      for (int i = 0; i < numberOfWifi; ++i) {  // Print SSID and RSSI for each network found.
        M5.Lcd.printf("%d:", i + 1);
        M5.Lcd.print(WiFi.SSID(i));
        M5.Lcd.printf("(%d)", WiFi.RSSI(i));

        ssidData.id = i ;
        WiFi.SSID(i).getBytes((unsigned char *)ssidData.essid,sizeof(ssidData.essid));
        strcpy(ssidData.bssid,"00:00:00:00:00:00");
        ssidData.rssi = WiFi.RSSI(i);
        ssidData.frequency = 0;
        ssidData.latitude = 43.058095;
        ssidData.longitude = 144.843528;
        strcpy(ssidData.datetime,"2023-01-01 00:00:00");
        delay(10);
        json = json + ssidData.getJson() + ",\n";
      }
      writeRecord((char *)json.c_str());
    }
    delay(500);
  }
  // Button C ////////////////////////////////////////////////////////////
  if (M5.BtnC.isPressed()) {  //If button C is pressed.
    M5.Lcd.clear();           //Clear the screen.
    M5.Lcd.println("Connect to Server\n");
    Serial.printf("function:loop, ButtunC Pressed\n");
    if(connectWiFi()){
      if(connectToServer()){
        M5.Lcd.printf("success.\n");
      } else
      {
        M5.Lcd.printf("failed to connect to server.\n");
      }
    }
    disconnectWiFi();
    deleteFile();
    Serial.printf("function:loop, ButtunC Pressed-exit\n");
  }

  smartDelay(200);
  if (millis() > 5000 && gps.charsProcessed() < 10){
    M5.Lcd.println(F("No GPS data received: check wiring"));  
  }
}
*/
static unsigned int checkUserAction(void){
  char  tempStr[256]={0};  
  unsigned int result = 0x00000000;

  M5.update();  //Check the status of the key.
  // Button A /////////////////////////////////////////////////////////////
  if (M5.BtnA.isPressed()) {  //If button A is pressed.
    MyWebApi  heatmapApi;
    String  echoMsg = "[{\"CLIENT\":\"ARDUINO\"}]";
    String  postMsg = "";
    int result; 
    int recordCount;   

    lowerTty.putString("Button A pressed.\n");
    
    heatmapApi.init((char *)SERVER_HOST);    
    if(connectWiFi()){
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
    disconnectWiFi();   
    result = result | 0x00000001;
  }
  if (M5.BtnB.isPressed()) {  //If button B is pressed.
    lowerTty.putString("Button B pressed.\n");
    result = result | 0x00000002;
  }
  if (M5.BtnC.isPressed()) {  //If button C is pressed.
    lowerTty.putString("Button C pressed.\n");
    lowerTty.putString("Dump SD " DATA_FILE_NAME "\n");
    dumpRecord();
    smartDelay(2000);

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
  char tmpDate[10 + 1];  // "YYYY/MM/DD"
  char tmpTime[8 + 1];   //  "hh:mm:ss"

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
bool connectWiFi() {
  bool ret = false;
  int i;

  WiFi.begin(HOME_SSID, HOME_SSID_PASS);
  for (i = 0; i < 60; i++) {
    delay(100);
    ret = (WiFi.status() == WL_CONNECTED);
    if (ret) break;
  }
  Serial.printf("function:connectWiFi, i=%d,return(%d)\n", i, ret);
  return ret;
}
void disconnectWiFi() {
  WiFi.disconnect();
  delay(100);
  Serial.printf("function:disconnectWiFi, exit\n");
}
int writeRecord(char *record) {
  int ret = 0;
  File file;
  const char *fileName = DATA_FILE_NAME;

  Serial.printf("function:writeRecord, ssid=%s\n", record);
  file = SD.open(fileName, FILE_APPEND);
  file.print(record);
  file.close();
  Serial.printf("function:writeRecord, return(%d)\n", ret);
  return ret;
}
int createPostMsg(String& postMsg ){
  const char *fileName = DATA_FILE_NAME;
  char *buf;
  int ptr = 0;
  int recordCount = 0;
  File file;

  Serial.printf("function:createPostMsg\n");
  file = SD.open(fileName, FILE_READ);
  Serial.printf("function:createPostMsg, available=%d\n", file.available());

  buf = (char *)malloc(file.size() + 2);  // top='[' ,tail=0x00
  if(buf == NULL){
    lowerTty.putString((char *)"createPostMsg: failed to memory allocation.\n");
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
void convertMacAddr(uint8_t *bin, char *str) {
  sprintf(str, "%02X:%02X:%02X:%02X:%02X:%02X", bin[0], bin[1], bin[2], bin[3], bin[4], bin[5]);
}
