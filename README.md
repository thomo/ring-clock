LED RGB Ring Clock
==================

Used 3rd party libraries
------------------------
* [FASTLED](https://fastled.io/) - downloaded 2014-10-15 - LED ring control
* [Time](http://www.pjrc.com/teensy/td_libs_Time.html) - downloaded 2014-10-11 - date/time handling - copy sub folder into Arduino libraries folder
* [DS1307RTC](http://www.pjrc.com/teensy/td_libs_DS1307RTC.html) - downloaded 2014-12-13 - Real Time Clock support
* [OneWire](http://www.pjrc.com/teensy/td_libs_OneWire.html) - downloaded 2014-12-13 - read temperature sensor
* [DallasTemperature](https://github.com/milesburton/Arduino-Temperature-Control-Library) - downloaded 2014-12-13 - convert sensor value into temperature
* [TimerOne](https://www.pjrc.com/teensy/td_libs_TimerOne.html) - downloaded 2014-12-13 - Timer interrupt support
* [DCF77](http://thijs.elenbaas.net/downloads/?did=1) - v0.9.8, downloaded 2014-11-01 - only copy DCF77 subfolder into Arduino libraries folder, I also must change DCFSplitTime to 130 (in DCF77.h)

### Hardware setup


Repository Content
==================

+ RingClock - Arduino source code
+ libs - used 3rd party libraries
