M5Stack Atom Matrix Timezones

To the Port A of the AtomPortABC module are connected:
1. M5Stack GROVE HUB;
2. M5Stack OLED (SH1107);
3. M5Stack RTC (HYM8563);

By Pressing the button on top of the LED display (pressing the screen down), the color of the 5x5 LEDs will change.
If the button is not pressed, the sketch will animate the LED display of the Atom Matrix, as if it was a countdown indicator.
At the moment all LEDs are dimmed (off: BLACK), the sketch will switch to the next Timezone and will set the external RTC unit acordingly.
In four steps the OLED will show: 
   1) Time zone name and city, for example: "Europe" and "Lisbon"; 
   2) the word "Zone" and the Timezone in letters, for example "CEST", and the offset to UTC, for example "+0100";
   3) date info, for example "Monday September 30"; 
   4) time info, for example: "20:52:28 in: Lisbon".

On reset the Arduino Sketch will try to connect to a WiFi Access Point. If successful the sketch will next connect to a NTP server and download the current datetime stamp.
If NTP is connected, the external RTC unit will be set to the NTP datetime stamp.
Next the sketch will display date and time (UTC), taken from the external RTC, onto the OLED display. Every second the date and time will be updated.
Because the external RTC Unit has a built-in battery, the datetime set will not be lost when power is lost.

File secret.h :

Update the file secret.h as far as needed:
```
 a) your WiFi SSID in SECRET_SSID;
 b) your WiFi PASSWORD in SECRET_PASS;
 c) your timezone in SECRET_NTP_TIMEZONE, for example: Europe/Lisbon;
 d) your timezone code in SECRET_NTP_TIMEZONE_CODE, for example: WET0WEST,M3.5.0/1,M10.5.0;
 e) the name of the NTP server of your choice in SECRET_NTP_SERVER_1, for example: 2.pt.pool.ntp.org.
```
 In the sketch is pre-programmed a map (dictionary), name ```time_zones```. This map contains six timezones:

```
    zones_map[0] = std::make_tuple("Asia/Tokyo", "JST-9");
    zones_map[1] = std::make_tuple("America/Kentucky/Louisville", "EST5EDT,M3.2.0,M11.1.0");
    zones_map[2] = std::make_tuple("America/New_York", "EST5EDT,M3.2.0,M11.1.0");
    zones_map[3] = std::make_tuple("America/Sao_Paulo", "<-03>3");
    zones_map[4] = std::make_tuple("Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0/3");
    zones_map[5] = std::make_tuple("Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3");
```

 After reset the sketch will load from the file ```secret.h``` the values of ```SECRET_NTP_TIMEZONE``` and ```SECRET_NTP_TIMEZONE_CODE```, 
 and replaces the first record in the map time_zones with these values from secret.h.

Docs

See the Monitor_output.txt

Images 

Hardware used:
- M5Stack Atom Matrix ESP32 Development Kit [info](https://shop.m5stack.com/products/atom-matrix-esp32-development-kit);
- M5Stack AtomPortABC base [info](https://docs.m5stack.com/en/unit/AtomPortABC);
- M5Stack GROVE HUB [info](https://docs.m5stack.com/en/unit/hub);
- M5Stack OLED unit [info](https://docs.m5stack.com/en/unit/oled);
- M5Stack RTC unit [info](https://shop.m5stack.com/products/real-time-clock-rtc-unit-hym8563)
