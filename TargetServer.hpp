/**
 * @file TargetServer.hpp
 * @brief サーバクラスヘッダ
 */

#ifndef TARGET_SERVER_HPP
#define TARGET_SERVER_HPP

#include <Arduino.h>
#include <WiFiClient.h>
#include <WebServer.h>

class TargetServer {
public:
  TargetServer() {
    _server = new WebServer(80);
  }
  TargetServer(IPAddress ip, int port = 80):_ip(ip), _port(port) {
    _server = new WebServer(ip, port);
  };
  ~TargetServer() {
    delete _server;
  };
  void on_shoot(void (*func)(WebServer *web_server)) {
    if (_server == nullptr) return;
    _server->on("/", [this, func](){func(_server);});
  };
  void begin(void) {
    if (_server == nullptr) return;
    _server->begin();
  }
  void handle_client(void) {
    if (_server == nullptr) return;
    _server->handleClient();
  }
private:
  IPAddress _ip = IPAddress(0, 0, 0, 0);
  int _port = 80;
  WebServer *_server = nullptr;
};

#endif // TARGET_SERVER_HPP
