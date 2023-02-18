#ifndef mywebapi_h
#define mywebapi_h

#include <M5Core2.h>
#include <string.h>

#define ADDRESS_SIZE  256
#define URI_SIZE      1024

#define ECHO      "/heatmap/echo.php"
#define POSTSSID  "/heatmap/postssid.php"

class MyWebApi {
  public:
    MyWebApi();
    ~MyWebApi();
    bool  init(char *serverAddress);
    bool  echo(String msg);
    int   postSsid(String msg);
  private:
    char  server[ADDRESS_SIZE];
    char  echoUri[URI_SIZE];
    char  postSsidUri[URI_SIZE];
};
#endif