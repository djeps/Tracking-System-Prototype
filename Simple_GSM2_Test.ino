#define ADC_RES 1024
#define PWRKEY 49
#define STATUS A0

int stat = 0;
String resp = "";

void setup()
{
    Serial.begin(9600);
    Serial.println("Starting MCU UART...");
    
    pinMode(PWRKEY, OUTPUT);
    pinMode(STATUS, INPUT);

    // Make sure the PWRKEY on the GSM2 Click board is at logic LOW
    digitalWrite(PWRKEY, LOW);
    delay(250); // This isn't really neccessary...

    // Put the PWRKEY of the GSM@ Click board at logic HIGH
    // and wait until STATUS is at logic HIGH
    digitalWrite(PWRKEY, HIGH);
    while (stat < (ADC_RES-1))
    {
        stat = analogRead(STATUS); // For an ADC with a 10 bit resolution 5V = 2^10-1
    }

    // The modem is properly powered up, and we can now send AT commands
    Serial.println("GSM modem is powered-up!");
    digitalWrite(PWRKEY, LOW);

    Serial.println("Starting GSM2 UART...");
    Serial1.begin(9600);
    
    Serial1.println( "AT" ); // Sending the 1st AT command
    
    // A mandatory 2-3 sec delay before expecting an answer according to QUECTEL's M95 documentation
    delay(3000);
    
    Serial1.println( "AT" ); // Sending the 2nd AT command

    while (Serial1.available())
    {
        char c = Serial1.read();
        resp += c;
    }

    resp[resp.length()-1] = '\0';
    Serial.println(resp); // The M95 modem should reply OK
}

void loop()
{
    
}
