#include "HttpServerUtils.h"

const char* getHttpRespHeader() {
  // Add "\r\n\r\n" to ensure proper header-body separation
  return "HTTP/1.1 200 OK\r\n"
         "Content-Type: text/html\r\n"
         "\r\n";  // This marks the end of headers
}

const char* getHTMLPage() {
  return R"(
<html>
  <body>
    <h1>Hello from MyLibrary!</h1>
    <p>This HTML page is returned as a multi-line string.</p>
  </body>
</html>
)";
}

