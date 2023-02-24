#ifndef inifile_h
#define inifile_h

#include <M5Core2.h>
#include <string.h>

class Inifile {
  public:
    Inifile();
    ~Inifile();
    bool    getValue(const char* filename, const char* key, String& value);
  private:
    String  getLine(File& file);
    String  getWord(String& str);    
};
#endif