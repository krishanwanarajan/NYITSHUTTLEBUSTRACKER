//Created By Krishan
//Code Inspired by Marco Schwartz

//Libraries
#include <Adafruit_SleepyDog.h>
#include <SoftwareSerial.h>
#include "Adafruit_FONA.h"
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_FONA.h"
#include <Time.h>

// LED pin
const int ledPin = 6;

//Latitude & Longitude
float initialLatitude;
float initialLongitude;
float latitude, longitude, speed_kph, heading, altitude;

//FONA pin configuration
#define FONA_RX     2
#define FONA_TX     3
#define FONA_RST    4

//FONA GPRS configuration
#define FONA_APN        "wholesale"
#define FONA_USERNAME   ""
#define FONA_PASSWORD   ""

//Server Configuration
#define SERVER          "104.143.38.56"
#define SERVERPORT      8124

// FONA instance & configuration
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);    // FONA software serial connection.
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);  


//*************************************************************************************************************************************
//*************************************************************************************************************************************
//*************************************************************************************************************************************


void setup() {
  // put your setup code here, to run once:

  // Initialize serial output.
  Serial.begin(115200);
  Serial.println(F("NYIT SHUTTLE TRACKER"));

    // Initialize the FONA module
  Serial.println(F("Initializing FONA....(may take 10 seconds)"));
  fonaSS.begin(4800);
  if (!fona.begin(fonaSS)) {
    halt(F("Couldn't find FONA"));
  }
  fonaSS.println("AT+CMEE=2");
  Serial.println(F("FONA is OK"));

  // Wait for FONA to connect to cell network (up to 8 seconds, then watchdog reset).
  Serial.println(F("Checking for network..."));
  while (fona.getNetworkStatus() != 1) {
   delay(500);
  }

  // Enable GPS.
  fona.enableGPS(true);

  // Start the GPRS data connection.
  Watchdog.reset();
  fona.setGPRSNetworkSettings(F(FONA_APN));
  fona.setGPRSNetworkSettings(F(FONA_APN), F(FONA_USERNAME), F(FONA_PASSWORD));
  delay(2000);
  Watchdog.reset();
  Serial.println(F("Disabling GPRS"));
  fona.enableGPRS(false);
  delay(2000);
  Watchdog.reset();
  Serial.println(F("Enabling GPRS"));
  if (!fona.enableGPRS(true)) {
    halt(F("Failed to turn GPRS on, resetting..."));
  }
  Serial.println(F("Connected to Cellular!"));

    // Initial GPS read
  bool gpsFix = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);
  initialLatitude = latitude;
  initialLongitude = longitude;

  

}


//*************************************************************************************************************************************
//*************************************************************************************************************************************
//*************************************************************************************************************************************


void loop() {
  // put your main code here, to run repeatedly:
  float latitude, longitude, speed_kph, heading, altitude;
  bool gpsFix = fona.getGPS(&latitude, &longitude, &speed_kph, &heading, &altitude);
  String bus_id = "1";

  Serial.print("Heading: ");
  printFloat(heading, 5);
  Serial.println("");

  Serial.print("Latitude: ");
  printFloat(latitude, 5);
  Serial.println("");

  Serial.print("Longitude: ");
  printFloat(longitude, 5);
  Serial.println("");


  String current_latitude = "";
  String current_longitude = "";
  String current_heading = "";
  String current_lat_long = "";
  

  current_latitude+=String(int(latitude))+ "."+String(getDecimal(latitude));
  current_longitude+=String(int(longitude))+ "."+String(getDecimal(longitude));
  current_heading+=String(int(heading))+ "."+String(getDecimal(heading));
  current_lat_long = bus_id + "," + current_heading + "," + current_latitude + "," + current_longitude;

  Serial.print("Cordinates: " + current_latitude + ", " + current_longitude );
  Serial.println("");
  
  
  
  //getme();
  post(current_lat_long);
  
  
}


//*************************************************************************************************************************************
//*************************************************************************************************************************************
//*************************************************************************************************************************************


void halt(const __FlashStringHelper *error) {
  Serial.println(error);
  while (1) {
    digitalWrite(ledPin, LOW);
    delay(100);
    digitalWrite(ledPin, HIGH);
    delay(100);
  }
}

void printFloat(float value, int places) {
  // this is used to cast digits 
  int digit;
  float tens = 0.1;
  int tenscount = 0;
  int i;
  float tempfloat = value;

    // make sure we round properly. this could use pow from <math.h>, but doesn't seem worth the import
  // if this rounding step isn't here, the value  54.321 prints as 54.3209

  // calculate rounding term d:   0.5/pow(10,places)  
  float d = 0.5;
  if (value < 0)
    d *= -1.0;
  // divide by ten for each decimal place
  for (i = 0; i < places; i++)
    d/= 10.0;    
  // this small addition, combined with truncation will round our values properly 
  tempfloat +=  d;

  // first get value tens to be the large power of ten less than value
  // tenscount isn't necessary but it would be useful if you wanted to know after this how many chars the number will take

  if (value < 0)
    tempfloat *= -1.0;
  while ((tens * 10.0) <= tempfloat) {
    tens *= 10.0;
    tenscount += 1;
  }


  // write out the negative if needed
  if (value < 0)
    Serial.print('-');

  if (tenscount == 0)
    Serial.print(0, DEC);

  for (i=0; i< tenscount; i++) {
    digit = (int) (tempfloat/tens);
    Serial.print(digit, DEC);
    tempfloat = tempfloat - ((float)digit * tens);
    tens /= 10.0;
  }

  // if no places after decimal, stop now and return
  if (places <= 0)
    return;

  // otherwise, write the point and continue on
  Serial.print('.');  

  // now write out each decimal place by shifting digits one by one into the ones place and writing the truncated value
  for (i = 0; i < places; i++) {
    tempfloat *= 10.0; 
    digit = (int) tempfloat;
    Serial.print(digit,DEC);  
    // once written, subtract off that digit
    tempfloat = tempfloat - (float) digit; 
  }
}

void post(String message) {
  // Post data to website
  uint16_t statuscode; 
  int16_t length;
  char url[] = "http://104.143.38.56:8124";
  char data[80];

  message.toCharArray(data, 80);

  flushSerial();
  Serial.println(url);
  Serial.print(F("Data to post: "));
  Serial.println(data);

  Serial.println(F("****"));
  if (!fona.HTTP_POST_start(url, F("application/json"), (uint8_t *) data, strlen(data), &statuscode, (uint16_t *)&length)) {
    Serial.println("Failed!");
    goto exitFn;
  }
  while (length > 0) {
    while (fona.available()) {
      char c = fona.read();
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
      loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */
      UDR0 = c;
#else
      Serial.write(c);
#endif

      length--;
      if (! length) break;
    }
  }
  Serial.println(F("\n****"));
  fona.HTTP_POST_end();
  exitFn:
  ; // compiler complains if this isn't here
    // nothing to do after goto, which helps exit function if http fails
}

//function to extract decimal part of float
long getDecimal(float val)
{
  int intPart = int(val);
  long decPart = 100000*(val-intPart); //I am multiplying by 1000 assuming that the foat values will have a maximum of 5 decimal places. 
                                    //Change to match the number of decimal places you need
  if(decPart>0)return(decPart);           //return the decimal part of float number if it is available 
  else if(decPart<0)return((-1)*decPart); //if negative, multiply by -1
  else if(decPart=0)return(00);           //return 0 if decimal part of float number is not available
}

void getme() {
  // read website URL
        uint16_t statuscode;
        int16_t length;
        char url[80] = "http://104.143.38.56:8124";

        flushSerial();
        Serial.println(url);

        Serial.println(F("****"));
        if (!fona.HTTP_GET_start(url, &statuscode, (uint16_t *)&length)) {
          Serial.println("Failed!");
          goto exitFn;
        }
        while (length > 0) {
          while (fona.available()) {
            char c = fona.read();

            // Serial.write is too slow, we'll write directly to Serial register!
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
            loop_until_bit_is_set(UCSR0A, UDRE0); /* Wait until data register empty. */

            
            UDR0 = c;
#else
            Serial.write(c);
#endif
            length--;
            if (! length) break;
          }
        }
        Serial.println(F("\n****"));
        fona.HTTP_GET_end();
        exitFn:
        ; // compiler complains if this isn't here
          // nothing to do after goto, which helps exit function if http fails
}



void flushSerial() {
  while (Serial.available())
    Serial.read();
}




