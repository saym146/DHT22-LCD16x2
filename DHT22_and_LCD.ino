#include <LiquidCrystal.h>
#include <DHT.h>;

//Constants
#define DHTPIN 7     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)

DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor for normal 16mhz Arduino

// Setup the LiquidCrystal library with the pin numbers we have
// physically connected the module to.
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

//Variables
float hum;  //Stores humidity value
float temp; //Stores temperature value
float maxHum;
float minHum;
float maxTemp;
float minTemp;
int lastUpdate = 0;  
int currentIndex = 0; 


void setup() {  
  // Setup the number of columns and rows that are available on the LCD. 
  lcd.begin(16, 2);
  //Start DHT22 
  dht.begin();
}

void loop() {  
      
    temp =  GetTemperature();
    hum =   GetHumidity();

    if(isnan(temp) != 1 && isnan(hum) != 1)
    {
      maxTemp = max(maxTemp,temp);
      minTemp = minTemp == 0? maxTemp : min(minTemp,temp);
      
      maxHum = max(maxHum,hum);
      minHum = minHum == 0? maxHum : min(minHum,hum);
      
      lcd.setCursor(0, 0);
      lcd.print("T:"+String(temp,1)+" "+String(maxTemp,1)+"/"+String(minTemp,1));
      lcd.setCursor(0, 1);
      lcd.print("H:"+String(hum,1)+" "+String(maxHum,1)+"/"+String(minHum,1));
    }
    else{
      lcd.setCursor(2,0);
      lcd.print("No DHT Found!!");
      //For moving sentence
//      for(int i=15;i>=0;i--){
//        lcd.setCursor(i,0);
//        lcd.print("No DHT Found");
//        delay(500);
//      }
    }
}

float GetHumidity()
{
  return dht.readHumidity();
}

float GetTemperature()
{
  return dht.readTemperature();
}
