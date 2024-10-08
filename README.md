M5Stack Atom Matrix Timezones

Important credit:

I was only able to create and successfully finish this project with the great help of Microsoft AI assistant CoPilot.
CoPilot helped me correcting wrong C++ code fragments. It suggested C++ code fragments. CoPilot gave me, in one Q&A session, a "workaround" 
for certain limitation that the Arduino IDE has with the standard C++ specification. And CoPilot gave it's answers at great speed of response.
There were no delays. The answers were instantaneous! Thank you CoPilot. Thank you Microsoft for this exciting new tool!

Hardware setup:

A M5Stack Atom Matrix, on top of a M5Stack AtomPortABC base. To the Port A of the AtomPortABC base are connected:
1. M5Stack GROVE HUB;
2. M5Stack OLED (SH1107);
3. M5Stack RTC (HYM8563);

By Pressing the button on top of the LED display (pressing the screen down), the next Timezone in the map ```zones_map``` will be used and the color of the 5x5 LEDs of the Atom Matrix display will change.
If the button is not pressed, the sketch will animate the LED display of the Atom Matrix, as if it was a countdown indicator.
At the moment all LEDs are dimmed (off: BLACK), the sketch will switch to the next Timezone and will set the external RTC unit acordingly.
In four steps the OLED will show: 
   1) Time zone continent and city, for example: "Europe" and "Lisbon"; 
   2) the word "Zone" and the Timezone in letters, for example "CEST", and the offset to UTC, for example "+0100";
   3) date info, for example "Monday September 30 2024"; 
   4) time info, for example: "20:52:28 in: Lisbon".

On reset the Arduino Sketch will try to connect to the WiFi Access Point of your choice (set in secret.h). If successful the sketch will next connect to a NTP server of your choice, then download the current datetime stamp.
If NTP is connected, the external RTC unit will be set to the NTP datetime stamp with the local time for the current Timezone.
Next the sketch will display date and time of the current Timezone, taken from the external RTC, onto the OLED display. Every second the date and time will be updated.
Because the external RTC Unit has a built-in battery, the datetime set will not be lost when power is lost.

File secret.h:

Update the file secret.h as far as needed:
```
 a) your WiFi SSID in SECRET_SSID;
 b) your WiFi PASSWORD in SECRET_PASS;
 c) your timezone in SECRET_NTP_TIMEZONE, for example: Europe/Lisbon;
 d) your timezone code in SECRET_NTP_TIMEZONE_CODE, for example: WET0WEST,M3.5.0/1,M10.5.0;
 e) the name of the NTP server of your choice in SECRET_NTP_SERVER_1, for example: 2.pt.pool.ntp.org.
```
 In the sketch is pre-programmed a map (dictionary), name ```zones_map```. This map contains six timezones:

```
    zones_map[0] = std::make_tuple("Asia/Tokyo", "JST-9");
    zones_map[1] = std::make_tuple("America/Kentucky/Louisville", "EST5EDT,M3.2.0,M11.1.0");
    zones_map[2] = std::make_tuple("America/New_York", "EST5EDT,M3.2.0,M11.1.0");
    zones_map[3] = std::make_tuple("America/Sao_Paulo", "<-03>3");
    zones_map[4] = std::make_tuple("Europe/Amsterdam", "CET-1CEST,M3.5.0,M10.5.0/3");
    zones_map[5] = std::make_tuple("Australia/Sydney", "AEST-10AEDT,M10.1.0,M4.1.0/3");
```

 After reset the sketch will load from the file ```secret.h``` the values of ```SECRET_NTP_TIMEZONE``` and ```SECRET_NTP_TIMEZONE_CODE```, 
 and replaces the first record in the map ```zones_map``` with these values from secret.h.

Docs:

See the Monitor_output.txt

Images: 

See a snapshot image taken from a video I creeated of the sketch running. I uploaded this video to Irmgur,
however, until this moment, I wasn't able to create a correct link to put in this README.

Links to product pages of the hardware used:

- M5Stack Atom Matrix ESP32 Development Kit [info](https://shop.m5stack.com/products/atom-matrix-esp32-development-kit);
- M5Stack AtomPortABC base [info](https://docs.m5stack.com/en/unit/AtomPortABC);
- M5Stack GROVE HUB [info](https://docs.m5stack.com/en/unit/hub);
- M5Stack OLED unit [info](https://docs.m5stack.com/en/unit/oled);
- M5Stack RTC unit [info](https://shop.m5stack.com/products/real-time-clock-rtc-unit-hym8563)

Known problem:
The sketch halts after running for a few hours. I suspect that this is caused by a memory leak. I am investigating it.


