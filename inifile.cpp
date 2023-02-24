/*
  Configuration file class
  Example 
    File name = \system.ini
    ---
    Line 1:# (<- # is comment)
    Line 2:; (<- ; is comment)
    Line 3:HOGE 123 (<- Key="HOGE", Value="123")

*/

#include "inifile.h"
#include "debugTool.h"
#include <string.h>
// Constructor //////////////////////////////////////////////////////////////
Inifile::Inifile(){
}
// Destructor ////////////////////////////////////////////////////////////////
Inifile::~Inifile(){
}
// getValue //////////////////////////////////////////////////////////////////
bool Inifile::getValue(const char* filename, const char* key, String& value){
  File  file;
  char    c;
  String  str;
  String  word;
  String  keyString = key;

  Serial.printf("Inifile::getValue : filename=%s, key=%s\n",filename,key);
  file = SD.open(filename, FILE_READ);
  if(!file){
    Serial.printf("Inifile::get : File open error.\n");
    goto ERROR_NO_FILE;
  }
  str="";
  while (file.available() > 0) {
    // Read 1 line
    str = getLine(file);
    if(str.length()==0 || str.charAt(0)=='#' || str.charAt(0)==';'){
      // this line is comment
      ;
    } else {
      // Get first word
      word = getWord(str);
      if(word.equalsIgnoreCase(key)){
        //  Get second word 
        Serial.printf("Inifile::getValue : Find key(%s)\n",key);
        value = getWord(str);
        break;
      }
    }
    // next line...
    str="";
  }
  file.close();

  Serial.printf("Inifile::getValue : value=%s\n",value.c_str());
  return (value.length() > 0);
// Error proc
ERROR_NO_FILE:
  return false;
}

// Get first word at given string.
// and this function remove first word from given string.
String Inifile::getWord(String& str){
  String word="";
  int i;

  Serial.printf("Inifile::getWord : str =%s\n",str.c_str());
  str.trim();
  for(i=0;i<str.length();i++){
    if(str.charAt(i)==' ' || str.charAt(i)=='\t'){
      // char is delimiter
      break;
    } else {
      word = word + str.charAt(i);
      str.setCharAt(i,' ');
    }
  }
  str.trim();
  Serial.printf("Inifile::getWord : word =%s\n",word.c_str());
  return word;
}
// Get 1 Line form file
//  Note : This function move position of the file.
String Inifile::getLine(File& file){
  String  str="";
  char    c;

  while (file.available() > 0) {
    // Read 1 line
    c = file.read();
    if( c=='\n' || c=='\r' ){
      // end of line
      break;
    } else {
      str += c;
    }
  }
  Serial.printf("Inifile::getLine : str =%s\n",str.c_str());
  return str;
}