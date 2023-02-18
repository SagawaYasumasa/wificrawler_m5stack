#ifndef tty_h
#define tty_h

#include <M5Core2.h>
#include <string.h>

#define DEFAULT_FONT_SIZE   1
#define DEFAULT_FONT_WIDTH  6
#define DEFAULT_FONT_HEIGHT 8

class Tty {
  public:
    Tty();
    ~Tty();
    bool  init(int x, int y, int width, int height, int foregroundColor, int backgroundColor);
    void  clear();
    void  putString(char *str);
    void  putString(const char *str);
    void  putChar(char);
    void  writeLine(int row, char *str);
    void  writeLine(int row, const char *str);
    void  writeChar(int x, int y, char c);
    void  locate(int x, int y);
    int   getRowSize(void);
    int   getColumnSize(void);
  private:
    int   viewX1,viewY1,viewX2,viewY2;
    int   viewWidth,viewHeight;
    int   fgColor, bgColor;
    int   fontSize;
    int   fontWidth, fontHeight;
    int   rowSize, columnSize;  // character size
    int   cPosX,cPosY;    // character position
    char  *charVRAM;
    void  writeVRAM(int x,int y, char c);
    char  readVRAM(int x, int y);
    void  writeLineEntity(int row, char *str);
 //   char  charVRAM[DISPLAY_COLUMN_SIZE][DISPLAY_ROW_SIZE];  // charVRAM[X][Y]
};
#endif