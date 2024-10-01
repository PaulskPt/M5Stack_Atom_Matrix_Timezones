/*
*  Test sketch for M5Stack 1.3 inch OLED SH1107 display with 128 x 64 pixels
*  and RTC unit (8563)
*  by @PaulskPt 2024-09-19
*  Units: OLED and RTC connected via a GROVE HUB to PortA of a M5Stack AtomPortABC base with on top a M5Stack Atom Matrix
*
* About i2c_scan with following units connected to Atrom PortABC, Port A: RTC unit and OLED unit:
* I2C device found at address 0x3c !  = OLED unit
* I2C device found at address 0x51 !  = RTC unit
* I2C device found at address 0x68 !  = 6-axis IMU sensor (MPU6886) (builtin the ATOM Matrix)
*
*/
#include <WiFi.h>
#include <TimeLib.h>
#include <time.h>
#include <DateTime.h> // See: /Arduino/libraries/ESPDateTime/src
#include <M5Atom.h>
#include <Unit_RTC.h>  
#include <M5UnitOLED.h>
#include <M5GFX.h>
#include <driver/adc.h>
#include <FastLED.h>
#include "secret.h"
// Following 6 includes needed for creating, changing and using map time_zones
#include <iostream>
#include <map>
#include <memory>
#include <array>
#include <string>
#include <tuple>
#include <iomanip>
#include <cstring> // For strcpy

namespace {  // anonymous namespace (also known as an unnamed namespace)

// LEDs display
#define NUM_LEDS 25
//#define DATA_PIN 27
CRGB leds[NUM_LEDS];

#define WIFI_SSID     SECRET_SSID // "YOUR WIFI SSID NAME"
#define WIFI_PASSWORD SECRET_PASS //"YOUR WIFI PASSWORD"
#define NTP_TIMEZONE  SECRET_NTP_TIMEZONE // for example: "Europe/Lisbon"
#define NTP_TIMEZONE_CODE  SECRET_NTP_TIMEZONE_CODE // for example: "WET0WEST,M3.5.0/1,M10.5.0"
#define NTP_SERVER1   SECRET_NTP_SERVER_1 // for example: "0.pool.ntp.org"
#define NTP_SERVER2   "1.pool.ntp.org"
#define NTP_SERVER3   "2.pool.ntp.org"

bool lStart = true;
bool use_local_time = false; // for the external RTC    (was: use_local_time = true // for the ESP32 internal clock )
struct tm timeinfo;
bool use_timeinfo = true;

// 128 x 64
static constexpr const int vert[] = {0, 30, 60};

unsigned long start_t = millis();

bool ledDisplayHorizontal = true;
bool LedTimeToChangeColor = false;
int LedCountDownCounter = NUM_LEDS;
int LedNrDone = 0;


uint8_t FSM = 0;  // Store the number of key presses
int connect_try = 0;
int max_connect_try = 10;

Unit_RTC RTC(0x51);

rtc_time_type RTCtime;
rtc_date_type RTCdate;

volatile bool buttonPressed = false;

M5UnitOLED display(25, 21, 400000, 0, 0x3c);

M5Canvas canvas(&display);

int zone_idx = 0;
const int zone_max_idx = 6;
// I know Strings are less memory efficient than char arrays, 
// however I have less "headache" by using Strings. E.g. the string.indexOf()
// and string.substring() functions make work much easier!

} // end of namespace

std::map<int, std::tuple<std::string, std::string>> zones_map;

void create_maps(void) 
{
  zones_map[0] = std::make_tuple("Asia/Tokyo", "JST-9");
  zones_map[1] = std::make_tuple("America/Kentucky/Louisville", "EST5EDT,M3.2.0,M11.1.0");
  zones_map[2] = std::make_tuple("America/New_York", "EST5EDT,M3.2.0,M11.1.0");
  zones_map[3] = std::make_tuple("America/Sao_Paulo", "<-03>3");
  zones_map[4] = std::make_tuple("Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0/3");
  zones_map[5] = std::make_tuple("Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3");

  // Iterate and print the elements
  /*
  std::cout << "create_maps(): " << std::endl;
  for (const auto& pair : zones_map)
  {
    std::cout << "Key: " << pair.first << ". Values: ";
    std::cout << std::get<0>(pair.second) << ", ";
    std::cout << std::get<1>(pair.second) << ", ";
    std::cout << std::endl;
  }
  */
}

void map_replace_first_zone()
{
  bool ret = false;
  int tmp_zone_idx = 0;
  std::string elem_zone_original;
  std::string elem_zone_code_original;
  std::string elem_zone  = std::get<0>(zones_map[tmp_zone_idx]);
  std::string elem_zone_code  = std::get<1>(zones_map[tmp_zone_idx]);
  std::string elem_zone_check;
  std::string elem_zone_code_check;
  
  elem_zone_original = elem_zone; // make a copy
  elem_zone_code_original = elem_zone_code;
  elem_zone = NTP_TIMEZONE;
  elem_zone_code = NTP_TIMEZONE_CODE;
  zones_map[0] = std::make_tuple(elem_zone, elem_zone_code);
  // Check:
  elem_zone_check  = std::get<0>(zones_map[tmp_zone_idx]);
  elem_zone_code_check  = std::get<1>(zones_map[tmp_zone_idx]);

  /*
  std::cout << "Map size before erase: " << myMap.size() << std::endl;

  for (int i = 0; i < 2; i++
  {
    // Get iterator to the element with key i
    auto it = zones_map.find(i);
    if (it != zones_map.end())
    {
        zones_map.erase(it);
    }
  std::cout << "Map size after erase: " << myMap.size() << std::endl;
  */
 
  std::cout << "map_replace_first_zone(): successful replaced the first record of the zone_map:\n" << std::endl;
  
  std::cout << "zone original: \"" << elem_zone_original.c_str() << "\""
    << ", replaced by zone: \"" << elem_zone_code_check.c_str()  << "\""
    << " (from file secrets.h)\n" 
    << std::endl;
  
  std::cout << "zone code original: \"" <<  elem_zone_code_original.c_str() << "\""
    << ", replaced by zone code: \"" << elem_zone_code_check.c_str() << "\""
    << std::endl;
}

void LedFillColor(CRGB c)
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    leds[i] = c;
  }
  FastLED.show();
}

int LedMatrix[25] = 
{
  20, 15, 10, 5, 0,
  21, 16, 11, 6, 1,
  22, 17, 12, 7, 2,
  23, 18, 13, 8, 3,
  24, 19, 14, 9, 4
};

void LedSetBlack()
{
  leds[LedCountDownCounter] = CRGB::Black; // Change color as needed
  FastLED.show();
}

void LedDownCounter()
{
  if (LedCountDownCounter >= 0)
    LedCountDownCounter--;
  else
    LedTimeToChangeColor = true;
}

void LedHorizDownCounter()
{
  int idx = LedMatrix[LedNrDone];
  leds[idx] = CRGB::Black;   // range 0-24
  FastLED.show();
  LedNrDone++;
  if (LedNrDone > NUM_LEDS)
    LedTimeToChangeColor = true;
}

std::string elem_zone_code_old;
bool zone_has_changed = false;

void setTimezone(void){
  std::shared_ptr<std::string> TAG = std::make_shared<std::string>("setTimezone(): ");

  std::cout << *TAG << std::flush;
  std::string elem_zone = std::get<0>(zones_map[zone_idx]);
  std::string elem_zone_code = std::get<1>(zones_map[zone_idx]);
  if (elem_zone_code != elem_zone_code_old)
  {
    const char s1[] = "has changed to: ";
    zone_has_changed = true;
    std::cout << "Timezone " << s1 << "\"" << elem_zone.c_str() << "\"" << std::endl;
    std::cout << "Timezone code " << s1 << "\"" << elem_zone_code.c_str() << "\"" << std::endl;
  }
  // Serial.printf("Setting Timezone to \"%s\"\n",elem_zone_code.c_str());
  setenv("TZ",elem_zone_code.c_str(),1);
  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
  // Check:
  std::cout << *TAG << "check environment variable TZ = \"" << getenv("TZ") << std::endl;
}

/*
  The getLocalTime() function is often used in microcontroller projects, such as with the ESP32, 
  to retrieve the current local time from an NTP (Network Time Protocol) server. 
  Here’s a simple example of how you can use this function with the ESP32:
*/
bool poll_NTP()
{
  std::shared_ptr<std::string> TAG = std::make_shared<std::string>("poll_NTP(): ");
  bool ret = false;
  if(getLocalTime(&timeinfo))
    ret = true;
  else
  {
    std::cout << *TAG << "Failed to obtain time " << std::endl;
    canvas.clear();
    canvas.setCursor(0, vert[2]);
    canvas.print(F("Failed to obtain time"));
    display.waitDisplay();
    canvas.pushSprite(&display, 0, (display.height() - canvas.height()) >> 1);
    delay(3000);
  }
  return ret;
}

bool initTime(void)
{
  bool ret = false;
  std::shared_ptr<std::string> TAG = std::make_shared<std::string>("initTime(): ");

  std::string elem_zone = std::get<0>(zones_map[zone_idx]);
  std::string elem_zone_code = std::get<1>(zones_map[zone_idx]);

  std::cout << *TAG << "Setting up time" << std::endl;
  std::cout << "zone       = \"" << elem_zone.c_str() << "\"" << std::endl;
  std::cout << "zone code  = \"" << elem_zone_code.c_str() << "\"" << std::endl;
  std::cout 
    << "NTP_SERVER1 = \"" << NTP_SERVER1 << "\", " 
    << "NTP_SERVER2 = \"" << NTP_SERVER2 << "\", "
    << "NTP_SERVER3 = \"" << NTP_SERVER3 << "\""
    << std::endl;

  setTimezone();  // was: zone_code[zone_idx]);  // Set the new time zone

  /*
  * See answer from: bperrybap (March 2021, post #6)
  * on: https://forum.arduino.cc/t/getting-time-from-ntp-service-using-nodemcu-1-0-esp-12e/702333/5
  */

#ifndef ESP32
#define ESP32 (1)
#endif

// See: /Arduino/libraries/ESPDateTime/src/DateTime.cpp, lines 76-80
#if defined(ESP8266)
  configTzTime(elem_zone_code.c_str(), NTP_SERVER1, NTP_SERVER2, NTP_SERVER3); 
#elif defined(ESP32)
  configTzTime(elem_zone_code.c_str(), NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);  // This one is use for the M5Stack Atom Matrix
#endif

  if(!getLocalTime(&timeinfo))
  {
    std::cout << *TAG << "Failed to obtain time from NTP" << std::endl;
  }
  else
  {
    std::cout << *TAG << "Got the time from NTP" << std::endl;
    // Now we can set the real timezone
    ret = true;
  }
  return ret;
}

bool set_RTC(void)
{
  bool ret = false;
  // Serial.print("set_RTC(): timeinfo.tm_year = ");
  // Serial.println(timeinfo.tm_year);
  if (timeinfo.tm_year + 1900 > 1900)
  {
    RTCtime.Hours   = timeinfo.tm_hour;
    RTCtime.Minutes = timeinfo.tm_min;
    RTCtime.Seconds = timeinfo.tm_sec;

    RTCdate.Year    = timeinfo.tm_year + 1900;
    RTCdate.Month   = timeinfo.tm_mon + 1;
    RTCdate.Date    = timeinfo.tm_mday;
    RTCdate.WeekDay = timeinfo.tm_wday;  // 0 = Sunday, 1 = Monday, etc.

    RTC.setDate(&RTCdate);
    RTC.setTime(&RTCtime);
    std::cout << "set_RTC(): external RTC has been set" << std::endl;
    std::cout << "Check: " << std::endl;
    poll_date_RTC();
    poll_time_RTC();
    ret = true;
  }
  return ret;
}

void poll_date_RTC(void)
{
  RTC.getDate(&RTCdate);
  std::cout << "RTC date: " 
    << std::setw(4) << String(RTCdate.Year).c_str() << "-" 
    << std::setfill('0') << std::setw(2) << String(RTCdate.Month).c_str() << "-" 
    << std::setfill('0') << std::setw(2) << String(RTCdate.Date).c_str() 
    << std::endl;
}

void poll_time_RTC(void)
{
  RTC.getTime(&RTCtime);
  std::cout << "RTC time: "
      << std::setfill('0') << std::setw(2) << String(RTCtime.Hours).c_str()   << ":" 
      << std::setfill('0') << std::setw(2) << String(RTCtime.Minutes).c_str() << ":" 
      << std::setfill('0') << std::setw(2) << String(RTCtime.Seconds).c_str() << std::endl;
}

void printLocalTime()  // "Local" of the current selected timezone!
{ 
  std::string elem_zone = std::get<0>(zones_map[zone_idx]);

  // Serial.print("printLocalTimer(): timeinfo.tm_year = ");
  // Serial.println(timeinfo.tm_year);
  if (timeinfo.tm_year + 1900 > 1900)
  {
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%A, %B %d %Y %H:%M:%S zone %Z %z", &timeinfo);
    std::cout << "Timezone: " << elem_zone.c_str() << ", " << buffer << std::endl;

  }
}

void prLedNrDone()
{
  Serial.printf("LedNrDone = %02d, LedMatrix[%d] = %02d, LedTimeToChangeColor = %02d\n", 
    LedNrDone, LedNrDone, LedMatrix[LedNrDone], LedTimeToChangeColor);
}

/* This function uses global var timeinfo to display date and time data.
   The function also displays timezone info.
   It also calls ck_Btn() four times to increase a "catch" of BtnA keypress.
*/
void disp_data(void)
{
  std::shared_ptr<std::string> TAG = std::make_shared<std::string>("disp_data(): ");
  int disp_data_delay = 1000;
  // For unitOLED
  int scrollstep = 2;

  canvas.clear();
  std::string elem_zone  = std::get<0>(zones_map[zone_idx]);
  std::string copiedString, copiedString2;
  std::string part1, part2, part3, part4;
  std::string partUS1, partUS2;
  int index, index2, index3 = -1;
  copiedString =  elem_zone;
  // Search for a first forward slash (e.g.: "Europe/Lisbon")
  index = copiedString.find('/');
  index3 = copiedString.find('_'); // e.g. in "New_York" or "Sao_Paulo"

  if (ck_Btn())
    return;

  if (index3 >= 0)
  {
    partUS1 = copiedString.substr(0, index3);
    partUS2 = copiedString.substr(index3+1);
    copiedString = partUS1 + " " + partUS2;  // replaces the found "_" by a space " "
  }
  if (index >= 0)
  {
    part1 = copiedString.substr(0, index);
    part2 = copiedString.substr(index+1);

    copiedString2 = part2.c_str();

    // Search for a second forward slash  (e.g.: "America/Kentucky/Louisville")
    index2 = copiedString2.find('/'); 
    if (index2 >= 0)
    {
      part3 = copiedString2.substr(0, index2);
      part4 = copiedString2.substr(index2+1);
    }
  }
  // =========== 1st view =================
  if (ck_Btn())
    return;
  // prLedNrDone();
  if (index >= 0 && index2 >= 0)
  {
    canvas.setCursor(0, vert[0]+5);
    canvas.scroll(-scrollstep, 0);
    canvas.print(part1.c_str());
    canvas.setCursor(0, vert[1]-2);
    canvas.print(part3.c_str());
    canvas.setCursor(0, vert[2]-10);
    canvas.print(part4.c_str());
  }
  else if (index >= 0)
  {
    canvas.setCursor(0, vert[0]+5);
    canvas.scroll(-scrollstep, 0);
    canvas.print(part1.c_str());
    canvas.setCursor(0, vert[1]);
    canvas.print(part2.c_str());
  }
  else
  {
    canvas.setCursor(0, vert[0]+5);
    canvas.print(copiedString.c_str());
  }
  display.waitDisplay();
  canvas.pushSprite(&display, 0, (display.height() - canvas.height()) >> 1);
  delay(disp_data_delay);
  if (ledDisplayHorizontal)
    LedHorizDownCounter();
  else
    LedDownCounter();
  if (LedTimeToChangeColor)
    return;
  // =========== 2nd view =================
  if (ck_Btn())
    return;
  // prLedNrDone();
  canvas.clear();
  canvas.setCursor(0, vert[0]+5);
  canvas.print("Zone");
  canvas.setCursor(0, vert[1]);
  canvas.print(&timeinfo, "%Z %z");
  display.waitDisplay();
  canvas.pushSprite(&display, 0, (display.height() - canvas.height()) >> 1);
  delay(disp_data_delay);
  if (ledDisplayHorizontal)
    LedHorizDownCounter();
  else
    LedDownCounter();
  if (LedTimeToChangeColor)
    return;
  // =========== 3rd view =================
  if (ck_Btn())
    return;
  // prLedNrDone();
  canvas.clear();
  canvas.setCursor(0, vert[0]+5);
  canvas.print(&timeinfo, "%A");  // Day of the week
  canvas.setCursor(0, vert[1]-2);
  canvas.print(&timeinfo, "%B %d");
  canvas.setCursor(0, vert[2]-10);
  canvas.print(&timeinfo, "%Y");
  display.waitDisplay();
  canvas.pushSprite(&display, 0, (display.height() - canvas.height()) >> 1);
  delay(disp_data_delay);
  if (ledDisplayHorizontal)
    LedHorizDownCounter();
  else
    LedDownCounter();
  if (LedTimeToChangeColor)
    return;
   // =========== 4th view =================
  if (ck_Btn())
    return;
  // prLedNrDone();
  canvas.clear();
  canvas.setCursor(0, vert[0]+5);
  canvas.print(&timeinfo, "%H:%M:%S");
  canvas.setCursor(0, vert[1]);
  if (index2 >= 0)
  {
    canvas.printf("in: %s\n", part4.c_str());
    std::cout << *TAG << "part4 = " << part4.c_str() << ", index2 = " << index2 << std::endl;
  }
  else
    canvas.printf("in: %s\n", part2.c_str());
  display.waitDisplay();
  canvas.pushSprite(&display, 0, (display.height() - canvas.height()) >> 1);
  delay(disp_data_delay);
  if (ledDisplayHorizontal)
    LedHorizDownCounter();
  else
    LedDownCounter();
  if (LedTimeToChangeColor)
    return;
}

/* the color order that these LEDs use isn't the RRGGBB found in HTML, but GGRRBB. */
void chg_matrix_clr(void)
{
  switch (FSM) 
  {
    case 0:
        LedFillColor(CRGB::Green);
        break;
    case 1:
        LedFillColor(CRGB::Red);
        break;
    case 2:
        LedFillColor(CRGB::Blue);
        break;
    case 3:
        LedFillColor(CRGB::White);
        break;
    case 4:
        LedFillColor(CRGB::Magenta);
        break;
    case 5:
        LedFillColor(CRGB::Orange);
        break;
    case 6:
        LedFillColor(CRGB::Black);
        break;
    default:
        break;
  }
  LedCountDownCounter = NUM_LEDS; // Reset counter
  FastLED.show();
}

bool connect_WiFi(void)
{
  std::shared_ptr<std::string> TAG = std::make_shared<std::string>("connect_WiFi(): ");
  bool ret = false;
  std::cout << std::endl << "WiFi: " << std::flush;
  WiFi.begin( WIFI_SSID, WIFI_PASSWORD );

  for (int i = 20; i && WiFi.status() != WL_CONNECTED; --i)
  {
    std::cout << "." << std::flush;
    delay(500);
  }
  if (WiFi.status() == WL_CONNECTED) 
  {
    ret = true;
    Serial.print(F("\r\n"));
    std::cout << "\r\n" << std::flush;
    std::cout << *TAG << "WiFi Connected to: " << WIFI_SSID << std::endl;
    IPAddress ip;
    ip = WiFi.localIP();
    std::cout << "IP address: " << std::hex << ip << std::endl;
    byte mac[6];
    WiFi.macAddress(mac);

    // Allocate a buffer of 18 characters (12 for MAC + 5 colons + 1 null terminator)
    char* mac_buff = new char[18];

    // Create a shared_ptr to manage the buffer with a custom deleter
    std::shared_ptr<char> bufferPtr(mac_buff, customDeleter);

    std::cout << *TAG << std::endl;

    // Format the MAC address into the buffer
    char* ptr = bufferPtr.get();
    for (int i = 0; i < 6; ++i) {
        if (i > 0) {
            *ptr++ = ':'; // Add colon between MAC address bytes
        }
        sprintf(ptr, "%02X", mac[i]);
        ptr += 2;
    }
    std::cout << "MAC: " << bufferPtr.get() << std::endl;
  }
  else
  {
    std::cout << "\r\n" << *TAG << "WiFi connection failed." << std::endl;
  }
  return ret;
}

void customDeleter(char* buffer) {
    // std::cout << "\nCustom deleter called\n" << std::endl;
    delete[] buffer;
}

void getID(void)
{
  uint64_t chipid_EfM = ESP.getEfuseMac(); // The chip ID is essentially the MAC address 
  char chipid[13] = {0};
  sprintf( chipid,"%04X%08X", (uint16_t)(chipid_EfM>>32), (uint32_t)chipid_EfM );
  std::cout << "\nESP32 Chip ID = " << chipid << std::endl;
  std::cout << "chipid mirrored (same as M5Burner MAC): " << std::flush;
  // Mirror MAC address:
  for (uint8_t i = 10; i >= 0; i-=2)  // 10, 8. 6. 4. 2, 0
  {
    // bytes 10, 8, 6, 4, 2, 0
    // bytes 11, 9, 7. 5, 3, 1
    std::cout << chipid[i] << chipid[i+1] << std::flush;
    if (i > 0)
      std::cout << ":" << std::flush;
    if (i == 0)  // Note: this needs to be here. Yes, it is strange but without it the loop keeps on running.
      break;     // idem.
  }
}

bool ck_Btn()
{
  M5.update();
  //if (M5.Btn.isPressed())
  if (M5.Btn.wasPressed())  // 100 mSecs
  {
    buttonPressed = true;
    return true;
  }
  return false;
}

void handleButtonPress()
{
  buttonPressed = true;
}

void setup(void) 
{
  // bool SerialEnable, bool I2CEnable, bool DisplayEnable
  // Not using I2CEnable because of fixed values for SCL and SDA in M5Atom.begin:
  M5.begin(true, false, true);  // Init Atom-Matrix(Initialize serial port, LED).

  /*
  * A workaround to prevent some problems regarding 
  * M5.Btn.IsPressed(), M5.Btn.IsPressedPressed(), 
  * M5.Btn,wasPressed() or M5.Btn.pressedFor(ms).
  * See: https://community.m5stack.com/topic/3955/atom-button-at-gpio39-without-pullup/5
  */
  adc_power_acquire(); // ADC Power ON

  attachInterrupt(digitalPinToInterrupt(ck_Btn()), handleButtonPress, FALLING);

  // FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.addLeds<WS2812, 27, GRB>(leds, NUM_LEDS);
  
  FastLED.setBrightness(20);
  FastLED.clear();
  FastLED.show();
  LedFillColor(CRGB::Green);
  FastLED.show();
  
  // See: https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/unit/oled.pdf
  display.init();
  display.setRotation(1);
  display.setTextColor(WHITE, BLACK);

  canvas.setColorDepth(1); // mono color
  canvas.setFont(&fonts::FreeSans9pt7b); // was: efontCN_14);
  canvas.setTextWrap(false);
  canvas.setTextSize(1);
  canvas.createSprite(display.width() + 64, 72);

  Serial.begin(115200);

  getID();

  std::cout << "\n\nM5Stack Atom Matrix Timezones test with units: OLED and RTC." << std::endl;

  create_maps();  // creeate zones_map

  RTC.begin();

  map_replace_first_zone();
  // NTP auto setting
  std::string elem_zone_code  = std::get<1>(zones_map[0]);
  configTzTime(elem_zone_code.c_str(), NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
  setTimezone();

  delay(1000);

  zone_idx = 0;
  chg_matrix_clr();  // Set Led display to Green

  /* Try to establish WiFi connection. If so, Initialize NTP, */
  if (connect_WiFi())
  {
    if (initTime())
    {
      if (set_RTC())
      {
        printLocalTime();
        disp_data();
      }
    }
  }
  else
    connect_try++;

  canvas.clear();
}

void loop(void)
{
  unsigned long interval_t = 5 * 60 * 1000; // 5 minutes
  unsigned long zone_chg_interval_t = 1 * 60 * 1000; // 1 minute
  unsigned long curr_t = 0L;
  unsigned long elapsed_t = 0L;
  bool dummy = false;
  bool zone_change = false;

  while (true)
  {
    if (!ck_Btn())
    {
      if (WiFi.status() != WL_CONNECTED) // Check if we're still connected to WiFi
      {
        // Serial.print(F("loop(): WiFi connection lost. Trying to reconnect..."));
        std::cout << "loop(): WiFi connection lost. Trying to reconnect..." << std::endl;
        if (!connect_WiFi())  // Try to connect WiFi
        {
          connect_try++;
        }

        if (connect_try >= max_connect_try)
        {
          std::cout << "\nWiFi connect try failed " << connect_try << "times. Going into infinite loop...\n" << std::flush;
          for(;;)
          {
            delay(5000);
          }
        }
      }
      if (WiFi.status() == WL_CONNECTED)
      {
        curr_t = millis();
        elapsed_t = curr_t - start_t;
        if (lStart || zone_has_changed || elapsed_t >= interval_t)
        {
          lStart = false;
          start_t = curr_t;
          // Poll NTP and set external RTC to synchronize it

          if (initTime())
          {
            if (set_RTC())
              zone_has_changed = false;  // reset flag
          }
        }
      }
    }
    if (LedTimeToChangeColor || buttonPressed || elapsed_t >= zone_chg_interval_t)
    {
      if (buttonPressed)
        std::cout << "\nButton pressed\n" << std::endl;
      /*
        Increases the FSM Atom Matrix Display color index.
      */
      FSM++;
      if (FSM >= 6)
        FSM = 0;
      /*
        Increase the zone_index, so that the sketch
        will display data from a next timezone in the map: time_zones.
      */
      zone_idx++;
      if (zone_idx >= zone_max_idx) 
        zone_idx = 0;
      chg_matrix_clr();
      initTime();
      // delay(50);
      // chg_matrix_clr();
      buttonPressed = false;
      LedTimeToChangeColor = false;
      LedCountDownCounter = NUM_LEDS; // Reset the counter
      LedNrDone = 0; // Reset this count also
    }
    printLocalTime();
    disp_data();
    //delay(1000);  // Wait 1 second
    dummy = ck_Btn();  // Read the press state of the key.
  }
}
