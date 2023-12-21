// ----------------------------
// INCLUDE DIRECTIVES/LIBRARIES
// ----------------------------

	#include <TinyGPS.h>

	
	
// ----------------------------
// DEFINE DIRECTIVES/MACROS
// ----------------------------

	#define PWRKEY_PIN 49 			// GSM2 Click board POWER 'key'
	#define STATUS_PIN A0 			// GSM2 Click board STATUS 'key'
	
	/*
    * Due to bad soldering, I had to re-assign the pins in use
    *
	
	#define POWER_ON_LED 51     	// POWER ON LED indicator
	#define GSM_OK_LED 50       	// GSM OK LED indicator
	#define GPS_OK_LED 52       	// GPS OK LED indicator
  
    */
	
	#define GSM_OK_LED 51         	// GSM OK LED indicator
	#define GPS_OK_LED 50         	// GPS OK LED indicator
	#define RESET_BUTTON 52			// RESET button connected to PB1 of the MEGA ADK
	#define RESET_SIGNAL 9			// RESET signal connected to PH6

	#define MCU_UART Serial
	#define GSM_UART Serial1
	#define GPS_UART Serial2
	
	#define ADC_RES 1024
	#define STATUS_TIMEOUT 3000

	
	
// ----------------------------
// GLOBAL VARIABLES
// ----------------------------

	int            g_stat = 0;      	 // GSM modem status
	String         g_resp = "";     	 // GSM modem response
	TinyGPS        g_gps;				 // GPS object
	bool           g_gpsData; 			 // Flag indicator that new GPS data is available
	unsigned long  g_gpsChars;      	 // Number of characters received; If 0, something is wrong with the HW setup/wiring and nothing has been received
    unsigned short g_gpsSentences;  	 // Number of characters received
    unsigned short g_gpsChecksum;   	 // The checksum error; If 0, all is OK
	unsigned long  g_smsCounter = 0; 	 // Send a SMS every 30 mins
	
	String g_smsNumber = "<recepient_mobile_number>"; // Test mobile number to send messages to every 30 mins

	unsigned long elapsedTime = 0;

// ----------------------------
// FUNCTION PROTOTYPES
// ----------------------------

	void pinSetup();
	void resetOnTimer(uint8_t button);
	
	void gsmFlushBuffer(HardwareSerial* gsm_uart);
	void gsmFillBuffer(HardwareSerial* gsm_uart);
	bool gsmSendCommandWithTimeout(HardwareSerial* gsm_uart, String at_command, unsigned int timeout);
	bool gsmSendCommand(HardwareSerial* gsm_uart, String at_command);
	bool gsmGetStatus(int status_pin);
	bool gsmPowerDown(HardwareSerial* gsm_uart);
	bool gsmPowerUp(HardwareSerial* gsm_uart);
	void gsmEchoOn(HardwareSerial* gsm_uart);
	void gsmEchoOff(HardwareSerial* gsm_uart);
	void gsmGetFirmwareVersion(HardwareSerial* gsm_uart);
	void gsmShowICCID(HardwareSerial* gsm_uart);
	void gsmShowIMEI(HardwareSerial* gsm_uart);
	void gsmSetTextingMode(HardwareSerial* gsm_uart);
	void gsmSetTextingMemoryToSIM(HardwareSerial* gsm_uart);
	void gsmGetTextingServiceCenter(HardwareSerial* gsm_uart);
	void gsmSendTextMessage(HardwareSerial* gsm_uart, String phone_no, String sms_text);

	bool gpsGetStatus(HardwareSerial* gps_uart, TinyGPS* gps);
	void gpsGetCoordinates(HardwareSerial* gps_uart, TinyGPS* gps, String* lat, String* lon, int millis_interval);
	
// ----------------------------
// EXECUTE ONCE ONLY
// ----------------------------

void setup()
{
    pinSetup(); 					// Setup the IO pins
	
    g_resp.reserve(256); 			// Reserve 256 bytes for the GSM output buffer
	
    MCU_UART.begin(9600); 			// Open the MCU serial COM port
    while(!MCU_UART)
	{ ;	}
    MCU_UART.println("MCU UART is open and ready!");
	
    GSM_UART.begin(9600); 			// Open the GSM modem serial COM port
    while(!GSM_UART)
	{ ;	}
    MCU_UART.println("GSM UART is open and ready!");
	
    GPS_UART.begin(9600); 			// Open the GPS modem serial COM port
    while(!GPS_UART)
	{ ;	}
    MCU_UART.println("GPS UART is open and ready!");
	
	gsmPowerDown(&GSM_UART);
    delay(1000);
    gsmPowerUp(&GSM_UART);
	
    //gsmEchoOn(&GSM_UART);
    //gsmGetFirmwareVersion(&GSM_UART);
    //gsmShowICCID(&GSM_UART);
    //gsmShowIMEI(&GSM_UART);
    gsmSetTextingMode(&GSM_UART);
}



// ----------------------------
// EXECUTE 'FOREVER'
// ----------------------------

void loop()
{
  resetOnTimer(RESET_BUTTON);
  
	if ( gsmGetStatus(STATUS_PIN) )
	{
		MCU_UART.println("#GSM_STATUS_OK;");
		digitalWrite(GSM_OK_LED, HIGH);
	}
	else
	{
		MCU_UART.println("#GSM_STATUS_KO;");
		digitalWrite(GSM_OK_LED, LOW);
	}
  
	if ( gpsGetStatus(&GPS_UART, &g_gps) )
	{
		String lat;
		String lon;
    
		g_smsCounter++;
		
		MCU_UART.println("#GPS_STATUS_OK;");
		digitalWrite(GPS_OK_LED, HIGH);	
		gpsGetCoordinates(&GPS_UART, &g_gps, &lat, &lon, 250);

		if ( g_smsCounter >= 360 ) // 360 ticks each 5 sec is exactly 30 min 
		{
			String smsText = String("Geo: ");
			smsText += "LAT=" + lat + ", " + "LON=" + lon;      
			gsmSendTextMessage(&GSM_UART, g_smsNumber, smsText);  
			g_smsCounter = 0;
		}
    
		delay(5000); // Delay until the next GPS coordinates refresh		
	}
	else
	{
		MCU_UART.println("#GPS_STATUS_CHECK_ANTENNA_OR_HW_WIRING;");
		digitalWrite(GPS_OK_LED, LOW);		
		delay(1500);
	}
}



// ----------------------------
// FUNCTION IMPLEMENTATION
// ----------------------------

void pinSetup()
{
	pinMode(PWRKEY_PIN, OUTPUT);
	pinMode(STATUS_PIN, INPUT);
	pinMode(GSM_OK_LED, OUTPUT);
	pinMode(GPS_OK_LED, OUTPUT);
	pinMode(RESET_BUTTON, INPUT);

	// Set pins initial state
	digitalWrite(PWRKEY_PIN, LOW);

	//digitalWrite(POWER_ON_LED, HIGH);
	digitalWrite(GSM_OK_LED, LOW);
	digitalWrite(GPS_OK_LED, LOW);
}

void resetOnTimer(uint8_t button)
{
	static uint8_t lastBtnState = LOW;
	uint8_t state = digitalRead(button);
  
	if (state != lastBtnState)
	{
		lastBtnState = state;

		if (state == LOW)
		{
			if (elapsedTime >= 2) // Due to internal delays(!!), through trials, this equates to 5sec. 4 equates to 10sec, 6 equates to 15sec, and so on...
			{
				pinMode(RESET_SIGNAL, OUTPUT);
				digitalWrite(RESET_SIGNAL, LOW);
			}

			elapsedTime = 0;
		}
	}
	else
	{
		if (state == HIGH)
		{
			elapsedTime++;
		}
	}

	delay(100);
}

void gsmFlush(HardwareSerial* gsm_uart)
{
	while (gsm_uart->available())
	{
		gsm_uart->read(); // 'Flush-out' everything that was read by the UART before issuing the next AT command!
	}
}

void gsmFillBuffer(HardwareSerial* gsm_uart)
{
    while (gsm_uart->available())
    {
        char c = gsm_uart->read();
        g_resp.concat(c);
    }

    MCU_UART.println(g_resp);
    g_resp = "";
    delay(250);
}

bool gsmSendCommandWithTimeout(HardwareSerial* gsm_uart, String at_command, unsigned int timeout)
{
    gsm_uart->println(at_command);
    delay(timeout);
    
    return true;
}

bool gsmSendCommand(HardwareSerial* gsm_uart, String at_command)
{
    return gsmSendCommandWithTimeout(gsm_uart, at_command, 0);
}

bool gsmGetStatus(int status_pin)
{
    bool gsm_status = false;
    unsigned int t1 = millis();
    unsigned int t2 = 0;
    unsigned int delta_t = 0;
    
    // Wait until STATUS is at logic HIGH or timeout has been reached
    while (true)
    {
        g_stat = analogRead(status_pin);
        t2 = millis();
        delta_t = t2 - t1;

        if ( g_stat == ADC_RES-1 )
        {
            gsm_status = true;
            break; // 1st possible condition to exit from the while loop
        }
        
        if ( delta_t >= STATUS_TIMEOUT )
        {
            break; // 2nd possible condition to exit from the while loop
        }
    }

    return gsm_status;
}

bool gsmPowerDown(HardwareSerial* gsm_uart)
{
    bool gsm_power = false;
    
    gsmSendCommandWithTimeout(gsm_uart, "AT+QPOWD=1", 5000);
    
    gsmFillBuffer(gsm_uart);
    
    return gsm_power;
}

bool gsmPowerUp(HardwareSerial* gsm_uart)
{
    bool gsm_power = false;
    
    // Set the PWRKEY to HIGH
    digitalWrite(PWRKEY_PIN, HIGH);
    
    while (g_stat < (ADC_RES-1))
    {
        g_stat = analogRead(STATUS_PIN);
    }
    
    delay(250);
    digitalWrite(PWRKEY_PIN, LOW);

    gsmSendCommandWithTimeout(gsm_uart, "AT", 3000);
    gsmSendCommandWithTimeout(gsm_uart, "AT", 1000); // At this point, we should get an answer 'OK'
    
    gsmFillBuffer(gsm_uart);
    
    return gsm_power;
}

void gsmEchoOn(HardwareSerial* gsm_uart)
{
    gsmSendCommandWithTimeout(gsm_uart, "ATE1", 250);

    gsmFillBuffer(gsm_uart);
}

void gsmEchoOff(HardwareSerial* gsm_uart)
{
    gsmSendCommandWithTimeout(gsm_uart, "ATE0", 250);

    gsmFillBuffer(gsm_uart);
}

void gsmGetFirmwareVersion(HardwareSerial* gsm_uart)
{
    gsmSendCommandWithTimeout(gsm_uart, "ATI", 1000);

    gsmFillBuffer(gsm_uart);
}

void gsmShowICCID(HardwareSerial* gsm_uart)
{
    gsmSendCommandWithTimeout(gsm_uart, "AT+QCCID", 1000);

    gsmFillBuffer(gsm_uart);
}

void gsmShowIMEI(HardwareSerial* gsm_uart)
{
    gsmSendCommandWithTimeout(gsm_uart, "AT+GSN", 1000);

    gsmFillBuffer(gsm_uart);
}

void gsmSetTextingMode(HardwareSerial* gsm_uart)
{
    gsmSendCommandWithTimeout(gsm_uart, "AT+CMGF=1", 250);
    gsmSendCommandWithTimeout(gsm_uart, "AT+CMGF?", 250);
    
    gsmFillBuffer(gsm_uart);
}

void gsmSetTextingMemoryToSIM(HardwareSerial* gsm_uart)
{
    gsmSendCommandWithTimeout(gsm_uart, "AT+CPMS=\"ME\"", 100);
    gsmSendCommandWithTimeout(gsm_uart, "AT+CPMS=?", 500);
    
    gsmFillBuffer(gsm_uart);
}

void gsmGetTextingServiceCenter(HardwareSerial* gsm_uart)
{
    gsmSendCommandWithTimeout(gsm_uart, "AT+CSCA?", 500);
    
    gsmFillBuffer(gsm_uart);
}

void gsmSendTextMessage(HardwareSerial* gsm_uart, String phone_no, String  sms_text)
{
    // The AT command to send SMS
    gsmSendCommandWithTimeout(gsm_uart, "AT+CMGS=\"" + phone_no + "\"", 500);
    // The actual SMS text
    gsmSendCommandWithTimeout(gsm_uart, sms_text, 500);
    // The Ctrl+Z terminator
    GSM_UART.write(26);
    delay(5000);
    MCU_UART.write(26); // Just for debugging to see the Ctrl+Z byte being sent
    gsmSendCommandWithTimeout(gsm_uart, "AT+CMGS=?", 500);

    gsmFillBuffer(gsm_uart);
}

bool gpsGetStatus(HardwareSerial* gps_uart, TinyGPS* gps)
{
	bool gps_status = false;
    
	g_gpsData = false;
	
	// We parse the GPS data each second
    for (unsigned long start = millis(); millis() - start < 1000;)
    {
        while (gps_uart->available())
        {
            char c = gps_uart->read();
            
            if (gps->encode(c)) // Did a new valid sentence come in?
            {
                g_gpsData = true;

                // Get GPS stats
                gps->stats(&g_gpsChars, &g_gpsSentences, &g_gpsChecksum);
            
                if (g_gpsChars != 0)
                {
                    gps_status = true;
                }
            }
            else
            {
              break;
            }
        }
    }
	
	return gps_status;
}

void gpsGetCoordinates(HardwareSerial* gps_uart, TinyGPS* gps, String* lat, String* lon, int millis_interval)
{
    // GPS coordinates are available
    if (g_gpsData)
    {      
        float         flat; // Lattitude
        float         flon; // Longitude
        String        slat;
        String        slon;
        unsigned long age;
        
        gps->f_get_position(&flat, &flon, &age);
  
        // Format the LAT and LON as strings
        slat = String(flat == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flat, 6);
        slon = String(flon == TinyGPS::GPS_INVALID_F_ANGLE ? 0.0 : flon, 6);
  
        MCU_UART.print("#GPS_CHARS=");
        MCU_UART.print(g_gpsChars);
        MCU_UART.println(";");
		delay(millis_interval);
		
        MCU_UART.print("#GPS_SENTENCES=");
        MCU_UART.print(g_gpsSentences);
        MCU_UART.println(";");
        delay(millis_interval);
		
        MCU_UART.print("#GPS_CSUM ERR=");
        MCU_UART.print(g_gpsChecksum);
        MCU_UART.println(";");
		delay(millis_interval);
		
        MCU_UART.print("#GPS_LAT=");
        MCU_UART.print(slat);
        MCU_UART.println(";");
        delay(millis_interval);
		
        MCU_UART.print("#GPS_LON=");
        MCU_UART.print(slon);
        MCU_UART.println(";");
        delay(millis_interval);

        *lat = slat;
        *lon = slon;
    }
}
