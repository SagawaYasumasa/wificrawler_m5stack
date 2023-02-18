#include "tty.h"
#include "debugTool.h"
#include <string.h>
// Constructor //////////////////////////////////////////////////////////////
Tty::Tty(){
}
// Destructor ////////////////////////////////////////////////////////////////
Tty::~Tty(){
  if(charVRAM == NULL) return;
  free(charVRAM);
  charVRAM = NULL;
}
bool Tty::init(int x, int y, int width, int height, int foregroundColor, int backgroundColor){
  Serial.printf("tty : x=%d, y=%d, width=%d, height=%d.\n",x,y,width,height);

  viewX1 = x;   // ViewArea Top Left X
  viewY1 = y;   // ViewArea Top Left Y
  viewX2 = x + width - 1;   // ViewArea Bottom Right X
  viewY2 = y + height - 1;  // ViewArea Bottom Right Y
  viewWidth = width;
  viewHeight = height;

  fontSize = DEFAULT_FONT_SIZE;
  fontWidth = DEFAULT_FONT_WIDTH;
  fontHeight = DEFAULT_FONT_HEIGHT;
  fgColor = foregroundColor;
  bgColor = backgroundColor;

  columnSize = viewWidth / fontWidth;
  rowSize = viewHeight / fontHeight;

  charVRAM = (char *)malloc(rowSize * columnSize * sizeof(char));
  if( charVRAM != NULL){
    clear();
  } else {
    Serial.printf("tty::init : failed to memory allocation.\n");
  }
  return (charVRAM != NULL);
}
// Clear TTY /////////////////////////////////////////////////////////////////
void Tty::clear(){
  if(charVRAM == NULL) return;

  cPosX=0;
  cPosY=0;
  memset(charVRAM,' ',rowSize*columnSize * sizeof(char));
  M5.Lcd.fillRect(viewX1,viewY1,viewX2,viewY2,bgColor);
}
// get Row & Column size /////////////////////////////////////////////////////
int Tty::getRowSize(void){
  return rowSize;
}
int Tty::getColumnSize(void){
  return columnSize;
}

// Location //////////////////////////////////////////////////////////////////
void Tty::locate(int x, int y){
  if(x >= 0 && x < columnSize && y >= 0 && y < rowSize){
    cPosX = x;
    cPosY = y;        
  }
}
// Write Line ////////////////////////////////////////////////////////////////
// Clear 1line & Write 1line.
void Tty::writeLine(int row, char *str){
  writeLineEntity(row, str);
}
void Tty::writeLine(int row, const char *str){
  writeLineEntity(row, (char *)str);
}
void Tty::writeLineEntity(int row, char *str){
  char *tempStr;
  int   size;
  int   x,y;

  y = (row >= 0 && row <rowSize) ? row : cPosY;
  size = (columnSize > strlen(str)) ? columnSize : strlen(str);

  tempStr=(char *)malloc((size+1)*sizeof(char));
  if(tempStr != NULL){
    memset(tempStr,' ',size);
    memcpy(tempStr, str, strlen(str));
    for(x=0;x<columnSize;x++){
      writeVRAM(x,y,*(tempStr+x));
      M5.Lcd.drawChar(x*fontWidth + viewX1, y*fontHeight+viewY1,
                        *(tempStr+x),fgColor,bgColor,fontSize);
    }
    free(tempStr);    
  } else {
    Serial.printf("tty::writeLineEntity : failed to memory allocation.\n");
  }    
}
// Write Character ///////////////////////////////////////////////////////////
void Tty::writeChar(int x, int y, char c){
  if( x >= 0 && x < columnSize && y >= 0 && y < rowSize){
    writeVRAM(x,y,c);
    M5.Lcd.drawChar(x*fontWidth + viewX1, y*fontHeight+viewY1,
                        c,fgColor,bgColor,fontSize);
  }    
}
// Put String ////////////////////////////////////////////////////////////////
void Tty::putString(char *str){
  int ptr=0;
  while(str[ptr] != 0){
    putChar(str[ptr]);
    ptr++;
  }
}
void Tty::putString(const char *str){
  int ptr=0;
  while(str[ptr] != 0){
    putChar(str[ptr]);
    ptr++;
  }
}
// Put Character /////////////////////////////////////////////////////////////
void Tty::putChar(char c){
  bool needLF = false;
  bool needCR = false;
  bool needRollup = false;
  int x,y;

  if(charVRAM == NULL) return;

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
        writeVRAM(cPosX,cPosY,c);
        M5.Lcd.drawChar(cPosX*fontWidth + viewX1, cPosY*fontHeight+viewY1,
                        c,fgColor,bgColor,fontSize);
        cPosX++;
        if(cPosX >= columnSize){
          needLF=true;
          needCR=true;
        }                                          
    }
    if(needCR){
      cPosX=0;
    }
    if(needLF){
      cPosY++;
      if(cPosY >= rowSize){
        needRollup=true;
        cPosY=rowSize-1;
      }
    }
    if(needRollup){
      // Scroll up charVRAM
      for(y=0;y<rowSize-1;y++){
        for(x=0;x<columnSize;x++){
          writeVRAM(x,y,readVRAM(x,y+1));          
        }
      }
      // Bottom Line
      for(x=0;x<columnSize;x++){
        writeVRAM(x,rowSize-1,' ');
      }
      // ReDraw display area
      for(y=0;y<rowSize;y++){
        for(x=0;x<columnSize;x++){
          M5.Lcd.drawChar(x*fontWidth + viewX1, y*fontHeight + viewY1,
                        readVRAM(x,y),fgColor,bgColor,fontSize);
        }
      }
    }
  }
}
void  Tty::writeVRAM(int x, int y, char c){
  *(charVRAM+(columnSize*y)+x)=c;
}
char  Tty::readVRAM(int x, int y){
  char  c = 0;
  c = *(charVRAM+(columnSize*y)+x);  
  return c;  
}