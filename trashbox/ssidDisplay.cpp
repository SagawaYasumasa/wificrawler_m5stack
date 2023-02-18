#include "ssidDisplay.h"
#include "debugTool.h"
#include <string.h>
// Constructor //////////////////////////////////////////////////////////////
SsidDisplay::SsidDisplay(){
  clear();
}
// Destructor ///////////////////////////////////////////////////////////////
SsidDisplay::~SsidDisplay(){

}

void SsidDisplay::clear(){
  cPosX=0;
  cPosY=0;
  memset(charVRAM,' ',sizeof(charVRAM));
  M5.Lcd.fillRect(ORIGIN_X,ORIGIN_Y,
    ORIGIN_X+(DISPLAY_COLUMN_SIZE*FONT_WIDTH)-1,
    ORIGIN_Y+(DISPLAY_ROW_SIZE*FONT_HEIGHT)-1,
    BLUE);
}
void SsidDisplay::print(int id,char *essid,char *bssid, long rssi ){
  char buff[256]={0};
  sprintf(buff,"%02d:%s %s RSSI=%d\n",id, essid, bssid, rssi);
  putString(buff);
//  M5.Lcd.printf("%02d:%s %s RSSI=%d\n", id, essid, bssid, rssi);
}
void SsidDisplay::putString(char *str){
  int ptr=0;
  while(str[ptr] != 0){
    putc(str[ptr]);
    ptr++;
  }
}
void  SsidDisplay::putc(char c){
  bool needLF = false;
  bool needCR = false;
  bool needRollup = false;
  int x,y;

  if((c==0x0a)||(c==0x0d)||((c>=0x20)&&(c<=0x7e))){
    switch(c){
      case  0x0a:
        needLF=true;
        needCR=true;
        break;
      case  0x0d:
        needCR=true;
        break;
      default:
        charVRAM[cPosX][cPosY]=c;
        M5.Lcd.drawChar(cPosX*FONT_WIDTH, (cPosY*FONT_HEIGHT)+OFFSET_VERTICAL,
                        c,TFT_WHITE,TFT_BLACK,FONT_SIZE);
        cPosX++;
        if(cPosX >= DISPLAY_COLUMN_SIZE){
          needLF=true;
          needCR=true;
        }                                          
    }
    if(needCR){
      cPosX=0;
    }
    if(needLF){
      cPosY++;
      if(cPosY >= DISPLAY_ROW_SIZE){
        needRollup=true;
        cPosY=DISPLAY_ROW_SIZE-1;
      }
    }
    if(needRollup){
      Serial.printf("Rollup\n");
      // Scroll up charVRAM
      for(y=0;y<DISPLAY_ROW_SIZE-1;y++){
        for(x=0;x<DISPLAY_COLUMN_SIZE;x++){
          charVRAM[x][y]=charVRAM[x][y+1];
        }
      }
      // Bottom Line
      for(x=0;x<DISPLAY_COLUMN_SIZE;x++){
        charVRAM[x][DISPLAY_ROW_SIZE-1]=' ';
      }
      // ReDraw display area
      for(y=0;y<DISPLAY_ROW_SIZE;y++){
        for(x=0;x<DISPLAY_COLUMN_SIZE;x++){
          M5.Lcd.drawChar(x*FONT_WIDTH, (y*FONT_HEIGHT)+OFFSET_VERTICAL,
                        charVRAM[x][y],TFT_WHITE,TFT_BLACK,FONT_SIZE);
        }
      }
    }
  }
}