### Simple_GPS_Test.ino

A simple sketch based on the original one (simple_test.ino) that comes with the TinyGPS library. What it does is establishes a connection with the GPS Click board and displays the latitude and longitude. That's it!

### Simple_GSM2_Test.ino

A simple sketch using the GSM2 Click board. Shows basic setup and how to power up the GSM modem.

### Simple_GSM2_Test2.ino

A more complete example using the GSM2 Click board. Includes several functions that could be put inside a library.

### Simple_GSM2_Test2_withEventHandlers.ino

Similar to the previous sketch but done slightly differently using custom events.

### TrackingSystem.ino

All of the above in one complete sketch to demonstrate my proof of concept. It's missing writing to a SD card, but the terminal output could be used to collect and stored data (temporally) with e.g. a RPi.
