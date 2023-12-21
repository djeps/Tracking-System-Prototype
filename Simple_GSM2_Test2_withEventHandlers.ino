// ----------------------------
// INCLUDE DIRECTIVES/LIBRARIES
//
   #include "EventManager.h"
//
// ----------------------------

// ----------------------------
// DEFINE DIRECTIVES/MACROS
//
   #define PWRKEY_PIN 49
   #define STATUS_PIN A0

   #define MCU_UART Serial
   #define GSM_UART Serial1
   #define ADC_RES 1024
   #define STATUS_TIMEOUT 3000
//
// ----------------------------

// ----------------------------
// GLOBAL VARIABLES
//
   int g_stat = 0;                // GMS modem status
   String g_resp = "";            // GSM modem response
   EventManager g_EventManager; // Global event manager
//
// ----------------------------

// ----------------------------
// FUNCTION PROTOTYPES
//
   void gsmResponse();
   void gsmResponseHandler(int event, int param);
   void pinSetup();
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
//
// ----------------------------

void setup()
{
    // Reserve 256 bytes/chars for the output buffer
    g_resp.reserve(256);

    // Add the GSM response listener to the event manager
    g_EventManager.addListener(EventManager::kEventUser0, gsmResponseHandler);

    // Open the MCU serial COM port
    MCU_UART.begin(9600);
    while(!MCU_UART) {;} // Loop until UART0 is open!
    MCU_UART.println("Starting MCU UART...");

    // Open the GSM modem serial COM port
    GSM_UART.begin(9600);
    while(!GSM_UART) {;} // Loop until UART1 is open!
    MCU_UART.println("Starting GSM2 UART...");

    // Setup the IO pins
    pinSetup();

    // GSM modem power procedure and list basic info
    gsmPowerDown(&GSM_UART);
    delay(1000);
    gsmPowerUp(&GSM_UART);
    gsmEchoOn(&GSM_UART);
    gsmGetFirmwareVersion(&GSM_UART);
    gsmShowICCID(&GSM_UART);
    gsmShowIMEI(&GSM_UART);
    gsmSetTextingMode(&GSM_UART);
    
    /*    
    gsmSetTextingMemoryToSIM(&GSM_UART);
    gsmGetTextingServiceCenter(&GSM_UART);
    */

    gsmSendTextMessage(&GSM_UART, "<recepient_mobile_number>", "Hello World!");
    
}

void loop()
{
    // Handle any events that are in the queue of the event manager
    g_EventManager.processEvent();
    // Add events into the queue
    gsmGotResponse();
}

void gsmResponseHandler(int event, int param)
{
    // If the GSM modem response buffer is not empty
    MCU_UART.println(g_resp);
    g_resp = "";
    delay(250);
}

void gsmGotResponse()
{
    if ( g_resp.length() > 0 )
    {
        g_EventManager.queueEvent(EventManager::kEventUser0, 0);
    }
}

void pinSetup()
{
    pinMode(PWRKEY_PIN, OUTPUT);
    pinMode(STATUS_PIN, INPUT);

    // Set pins initial state
    digitalWrite(PWRKEY_PIN, LOW);
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
            MCU_UART.println("GSM Status: OK.");
            gsm_status = true;
            break; // 1st possible condition to exit from the while loop
        }
        
        if ( delta_t >= STATUS_TIMEOUT )
        {
            MCU_UART.println("GSM Status: KO!");
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
    gsmSendCommandWithTimeout(gsm_uart, "AT+CPMS=\"SM\"", 100);
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
