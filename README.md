# GPSParse
A quick GPS NMEA sentence parser for Arduino/Teensy/C-compatible embedded system.

Give it an NMEA sentence via GPS_update(), and if it contains velocity and or location,
GPSParse will pull it out and hold on to it for you until you ask for it.
