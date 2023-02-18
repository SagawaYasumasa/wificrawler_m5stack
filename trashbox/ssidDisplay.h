#ifndef SsidDisplay_h
#define SsidDisplay_h

#include <M5Core2.h>
#include <string.h>

#define ESSID_SIZE  32
#define BSSID_SIZE  18
#define DATETIME_SIZE 19

#define FONT_SIZE   1
#define FONT_WIDTH  6
#define FONT_HEIGHT 8
#define OFFSET_VERTICAL   (FONT_HEIGHT*8)    // =8pixel * 8line
#define DISPLAY_ROW_SIZE  20
#define DISPLAY_COLUMN_SIZE (320/FONT_WIDTH)
#define ORIGIN_X    0
#define ORIGIN_Y    OFFSET_VERTICAL

class SsidDisplay {
  public:
    SsidDisplay();
    ~SsidDisplay();
    void  clear();
    void  print(int id,char *essid,char *bssid, long rssi );
    void  putString(char *str);
    void  putc(char);
  private:
    int   cPosX,cPosY;    // character position
    char  charVRAM[DISPLAY_COLUMN_SIZE][DISPLAY_ROW_SIZE];  // charVRAM[X][Y]
};
#endif