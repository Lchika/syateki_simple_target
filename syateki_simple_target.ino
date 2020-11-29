#include <memory>
#include <vector>

#include <Arduino.h>
#define ESP32 // for VSCode intelliSense
#include <M5StickC.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <WiFi.h>

#include "debug.h"
#include "irReceiver.hpp"
#include "TargetServer.hpp"

static void blink_led(int target_id, unsigned int time_ms = 100, unsigned int count = 3);

struct Target
{
  IrReceiver irReceiver;
  bool is_alive = true;
};

// network settings
static const IPAddress ip(192, 168, 100, 200);
static const IPAddress gateway(192, 168, 100, 1);
static const IPAddress subnet(255, 255, 255, 0);
static const char *ssid = "your-ssid";
static const char *password = "your-password";

//Target targets[TARGET_NUM] {};
std::vector<Target> targets;
std::unique_ptr<TargetServer> server;
// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver led_driver = Adafruit_PWMServoDriver();

void setup()
{
  M5.begin();
  M5.Axp.ScreenBreath(10);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setRotation(3);

  BeginDebugPrint();

  // start server and WiFi
  connect_ap();
  start_server();

  Wire.begin(32, 33);
  DebugPrint("Wire begun");

  init_target_drivers();

  led_driver.begin();
  led_driver.setPWMFreq(1600); // This is the maximum PWM frequency
  for (int i = 0; i < targets.size(); i++)
  {
    // turn off all leds
    led_driver.setPWM(i, 0, 4096);
  }
}

void loop()
{
  M5.update();
  server->handle_client();
  update_led();
  show_battery_info();
  delay(100);
}

static void update_led()
{
  for (int target_id = 0; target_id < targets.size(); target_id++)
  {
    // ignore to update led if target is dead
    if(!targets[target_id].is_alive) continue;
    if (is_active(target_id))
    {
      led_driver.setPWM(target_id, 4096, 0); // led on
    }
    else
    {
      led_driver.setPWM(target_id, 0, 4096); // led off
    }
  }
}

static bool is_active(int target_id)
{
  byte gun_num = targets[target_id].irReceiver.read();
  if (gun_num != 0)
  {
    return true;
  }
  return false;
}

static void init_target_drivers()
{
  int target_num = get_target_num();
  init_target_vector(target_num, targets);
}

static int get_target_num()
{
  int target_num = 0;
  while (!M5.BtnA.wasPressed())
  {
    M5.update();
    if (M5.BtnB.wasPressed())
    {
      DebugPrint("BtnB is Pressed")
          target_num++;
    }
    M5.Lcd.drawString("target_num: " + String(target_num), 0, 15);
    M5.Lcd.drawString("COUNT UP:btnB, ENTER:btnA", 0, 25);
    delay(50);
  }
  DebugPrint("BtnA is Pressed")
      M5.Lcd.drawString("                                     ", 0, 25);
  M5.Lcd.drawString("configured!", 0, 25);
  return target_num;
}

static void init_target_vector(int target_num, std::vector<Target> &v)
{
  for (int i = 0; i < target_num; i++)
  {
    Target t {};
    t.irReceiver = IrReceiver(8 + i);
    if (!t.irReceiver.is_connected())
    {
      DebugPrint("faild to connect irReceiver [" + String(i) + "]");
      M5.Lcd.drawString("Can't connect to slave(" + String(i) + ")!", 0, 25);
    }
    v.push_back(t);
  }
}

static void start_server()
{
  //server.reset(new TargetServer(ip, 80));
  server.reset(new TargetServer());
  server->on_shoot(handle_shoot);
  server->on_init(handle_init);
  server->begin();
}

static void handle_shoot(WebServer *server)
{
  DebugPrint("handle_shoot()");

  String shoot_gun_num_s = server->arg("gun_num");
  DebugPrint("gun_num: " + String(shoot_gun_num_s));
  if (shoot_gun_num_s == "")
  {
    response_to_center(*server, shoot_gun_num_s, 0);
    return;
  }

  int shoot_gun_num_i = shoot_gun_num_s.toInt();
  for (int target_id = 0; target_id < targets.size(); target_id++)
  {
    if (!targets[target_id].is_alive)
      continue;
    if (is_hit(target_id, shoot_gun_num_i))
    {
      response_to_center(*server, shoot_gun_num_s, shoot_gun_num_i);
      blink_led(target_id);

      targets[target_id].is_alive = false;
      // led on
      led_driver.setPWM(target_id, 4096, 0);
      return;
    }
  }
  response_to_center(*server, shoot_gun_num_s, 0);
  return;
}

static void handle_init(WebServer *server)
{
  for (int target_id = 0; target_id < targets.size(); target_id++)
  {
    // led off
    led_driver.setPWM(target_id, 0, 4096);
    targets[target_id].is_alive = true;
  }
  server->send(200, "text/plain", "initialized");
}

static void response_to_center(WebServer &server, String gun_num_s, int response_num)
{
  server.send(200, "text/plain", "target=" + String(response_num));
  if (gun_num_s == "")
  {
    gun_num_s = " ";
  }
  M5.Lcd.drawString("FROM: " + gun_num_s + ", RETURN: " + String(response_num), 0, 65);
}

static bool is_hit(int target_id, int shoot_gun_num_i)
{
  byte gun_num = targets[target_id].irReceiver.read();
  DebugPrint("target: " + String(target_id) + ", gun_num: " + String(gun_num));
  if (gun_num == shoot_gun_num_i)
  {
    return true;
  }
  return false;
}

static void blink_led(int target_id, unsigned int time_ms, unsigned int count)
{
  for (int i = 0; i < count; i++)
  {
    led_driver.setPWM(target_id, 4096, 0);
    delay(time_ms);
    led_driver.setPWM(target_id, 0, 4096);
    delay(time_ms);
  }
}

static void connect_ap()
{
  M5.Lcd.drawString("connecting...", 0, 0);
  WiFi.mode(WIFI_AP_STA);
  if (!WiFi.config(ip, gateway, subnet))
  {
    DebugPrint("STA Failed to configure");
  }
  WiFi.begin(ssid, password);
  unsigned int try_connect_count = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    try_connect_count++;
    if (try_connect_count > 30)
      break;
    delay(500);
    DebugPrint(".");
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    DebugPrint("");
    DebugPrint("WiFi connected.");
    DebugPrint("IP address: ");
    DebugPrint(WiFi.localIP().toString().c_str());
    M5.Lcd.drawString(WiFi.localIP().toString().c_str(), 0, 0);
  }
  else
  {
    DebugPrint("WiFi connect process time out.");
    M5.Lcd.drawString("WiFi NOT connected", 0, 0);
  }
}

static void show_battery_info()
{
  // バッテリー電圧表示
  // GetVbatData()の戻り値はバッテリー電圧のステップ数で、
  // AXP192のデータシートによると1ステップは1.1mV
  double vbat = M5.Axp.GetVbatData() * 1.1 / 1000;
  M5.Lcd.setCursor(0, 40);
  M5.Lcd.printf("Volt: %.2fV", vbat);

  // バッテリー残量表示
  // 簡易的に、線形で4.2Vで100%、3.0Vで0%とする
  int8_t bat_charge_p = int8_t((vbat - 3.0) / 1.2 * 100);
  if (bat_charge_p > 100)
  {
    bat_charge_p = 100;
  }
  else if (bat_charge_p < 0)
  {
    bat_charge_p = 0;
  }
  M5.Lcd.setCursor(0, 50);
  M5.Lcd.printf("Charge: %3d%%", bat_charge_p);
}