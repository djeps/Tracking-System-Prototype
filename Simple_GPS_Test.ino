// Vladimir DJEPOVSKI
// 30.11.2018
// 
// A slightly modified version of the original simple_test sketch that comes with TinyGPS
// TinyGPS author: Mikal HART
//
// TinyGPS library helps us parse the incomming GPS data
// Much better alternative than going the manual route
// i.e. sending ATcommands to the GPS modules and
// parsing them manually!

#include <TinyGPS.h>

TinyGPS gps;

void setup()
{
    Serial.begin(9600);  // Starting the UART interface for communicating with the host MCU
    Serial2.begin(9600); // Starting the UART interface for communicating the GPS module
}

void loop()
{
    bool           newData = false; // Flag indicator that new GPS data is available
    unsigned long  chars;           // Number of characters received; If 0, something is wrong with the HW setup/wiring and nothing has been received
    unsigned short sentences;       // Number of characters received
    unsigned short failed;          // The checksum error; If 0, all is OK

    // We parse GPS data each second
    for (unsigned long start = millis(); millis() - start < 1000;)
    {
        while (Serial2.available())
        {
            char c = Serial2.read();
            
            if (gps.encode(c)) // Did a new valid sentence come in?
            {
                newData = true;
            }
        }
    }

    // Get GPS stats
    gps.stats(&chars, &sentences, &failed);
    
    if (chars == 0)
    {
        Serial.println("*** No characters received from GPS: check wiring/HW setup ***");
        delay(1500);
        return;
    }
    
    // GPS coordinates are available
    if (newData)
    {
        float         flat; // Lattitude
        float         flon; // Longitude
        String        slat;
        String        slon;
        unsigned long age;
        
        gps.f_get_position(&flat, &flon, &age);
  
        // Format the LAT and LON as strings
        slat = String(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
        slon = String(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
  
        // Print some debug info on the MCU UART

        // Print some debug info on the MCU UART
        // I may disable most of this
        // as all I'm interested in really, are
        // the lattitude and longitude
        Serial.print("CHARS=");
        Serial.print(chars);
        Serial.print(";");
        
        Serial.print("SENTENCES=");
        Serial.print(sentences);
        Serial.print(";");
        
        Serial.print("CSUM ERR=");
        Serial.print(failed);
        Serial.print(";");

        Serial.print("LAT=");
        Serial.print(slat);
        Serial.print(";");
        
        Serial.print("LON=");
        Serial.print(slon);
        Serial.print(";");
        Serial.println();
  
        delay(5000); // As I won't be needing this updated each second, I could well increase it even more
    }
    else
    {
        Serial.print("*** CHECK your antenna conection. ");
        Serial.println("If the antenna is connected, MAKE SURE it's in open space ***");
        delay(1500);
    }
}
