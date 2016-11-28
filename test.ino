#include <stdio.h>
#include <string.h>
#include <Wire.h>                            // I2C control library
#include "ds3231.h"
#include "U8glib.h"
#include <SoftwareSerial.h>

#define BUFF_MAX 32
#define BUFF2_MAX 16
#define TEMP_UPDATE_INTERVAL 60

#define DEBUG

//realtime clock
#define DS3231_TEMPERATURE_MSB      0x11
#define DS3231_TEMPERATURE_LSB      0x12

//OLED PIN
//                 pinNumber      OLED Board pinName    arduino pinMap (sireal pin)
#define OLED_MOSI  11          // D1                    MOSI
#define OLED_SCK   13          // D0                    SCK
#define OLED_DC     9          // DC
#define OLED_CS    10          // CS                    SS
#define OLED_RESET 12          // RES                   MISO

// SDS011 TX -> Arduino D2, SDS011 RX -> Arduino D3
SoftwareSerial SDS011_Serial(2,3);

const char *dayWeekData[7] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
unsigned long prev = 10000, interval = 1000;
unsigned int Pm25 = 0;
unsigned int Pm10 = 0;

typedef struct _this_data {
  struct ts current_time;
} this_data;

this_data g_tData;

//init oled
U8GLIB_SSD1306_128X64 u8g(OLED_SCK, OLED_MOSI, OLED_CS, OLED_DC); 

const char *get_day_week(unsigned char data)
{
  const char *dayWeek = NULL;
  switch (data) {
    case 1:
      dayWeek = dayWeekData[0];
      break;
    case 2:
      dayWeek = dayWeekData[1];
      break;
    case 3:
      dayWeek = dayWeekData[2];
      break;
    case 4:
      dayWeek = dayWeekData[3];
      break; 
    case 5:
      dayWeek = dayWeekData[4];
      break;
    case 6:
      dayWeek = dayWeekData[5];
      break;
    case 7:
      dayWeek = dayWeekData[6];
      break; 
  }
  return dayWeek;
}

void ProcessSerialData()
{

  // uint8_t: system variable 1Byte Char.
  uint8_t mData = 0;
  uint8_t i = 0;
  uint8_t mPkt[10] = {0};
  uint8_t mCheck = 0;
#ifdef DEBUG
  Serial.println(SDS011_Serial.available());
#endif

 while (SDS011_Serial.available() > 0)
  {  
    // from www.inovafitness.com
    // packet format: AA C0 PM25_Low PM25_High PM10_Low PM10_High 0 0 CRC AB
     mData = SDS011_Serial.read();
     
     delay(2);//wait until packet is received
     
    if(mData == 0xAA)//head1 ok
     {
        mPkt[0] =  mData;
        mData = SDS011_Serial.read();
        if(mData == 0xc0)//head2 ok
        {
          mPkt[1] =  mData;
          mCheck = 0;
          for(i=0;i < 6;i++)//data recv and crc calc
          {
             mPkt[i+2] = SDS011_Serial.read();
             delay(2);
             mCheck += mPkt[i+2];
          }
          mPkt[8] = SDS011_Serial.read();
          delay(1);
          mPkt[9] = SDS011_Serial.read();
          if(mCheck == mPkt[8])//crc ok
          {
            SDS011_Serial.flush();       

            Pm25 = (uint16_t)mPkt[2] | (uint16_t)(mPkt[3]<<8);            
            Pm10 = (uint16_t)mPkt[4] | (uint16_t)(mPkt[5]<<8);
             
            // make exact value
            Pm25 = Pm25/10;
            Pm10 = Pm10/10;

            //if it overflows, set the limitation.            
            if(Pm25 > 9999)
             Pm25 = 9999;
            if(Pm10 > 9999)
             Pm10 = 9999;                           
            //get one good packet
             return;
          }
        }      
     }
   } 
}

#ifdef DEBUG
void serialDebugDisp(){
  Serial.print("Pm25:");      
  Serial.print(Pm25);   
 
  Serial.print("  Pm10:");
  Serial.print(Pm10);

  Serial.println("");  
  }

void print_time_to_lcd (struct ts *t, int temp)
{
  char tBuff[BUFF_MAX] = {0,};

  //snprintf(tBuff, BUFF_MAX, "%4d.%02d.%02d (%s) %02d:%02d:%02d %03d",
  //           t->year, t->mon, t->mday, get_day_week(t->wday), t->hour, t->min, t->sec, dim);
  //snprintf(tBuff, BUFF_MAX, "%02d:%02d %02dC %s%s %03d%c",
  //               t->hour, t->min, temp, "Normal", "Sunrising",  ((dim*100)/250), '%');
  snprintf(tBuff, BUFF_MAX, "%4d.%02d.%02d (%s) %02d:%02d:%02d",
             t->year, t->mon, t->mday, get_day_week(t->wday), t->hour, t->min, t->sec);
  Serial.println(tBuff);
}
#endif

#if 0
byte DS3231_get_MSB(){
  Wire.beginTransmission(DS3231_I2C_ADDR);
  Wire.write(DS3231_TEMPERATURE_MSB);
  Wire.endTransmission();

  Wire.requestFrom(DS3231_I2C_ADDR, 1);
  return Wire.read();
}

byte DS3231_get_LSB(){
  Wire.beginTransmission(DS3231_I2C_ADDR);
  Wire.write(DS3231_TEMPERATURE_LSB);
  Wire.endTransmission();

  Wire.requestFrom(DS3231_I2C_ADDR, 1);
  //temp_lsb = Wire.read() >> 6;
  return Wire.read() >> 6;
}
#endif

void draw(struct ts *t, int temp) {
  // graphic commands to redraw the complete screen should be placed here  
  //u8g.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
#if 0
  char tBuff[BUFF_MAX] = {0,};
  u8g.setFont(u8g_font_9x18r);
  snprintf(tBuff, BUFF_MAX, "%4d.%02d.%02d %s %02d:%02d:%02d",
             t->year, t->mon, t->mday, get_day_week(t->wday), t->hour, t->min, t->sec);
  u8g.drawStr( 0, 10, tBuff);
#endif

  char tBuff[BUFF2_MAX] = {0,};
  u8g.setFont(u8g_font_9x18r);
  snprintf(tBuff, BUFF2_MAX, "%02d:%02d:%02d", t->hour, t->min, t->sec);
  u8g.drawStr( 0, 18, tBuff);

  //u8g.setFont(u8g_font_fub11n); //24x21
  //snprintf(tBuff, BUFF2_MAX, "PM25 : %d", Pm25);
  snprintf(tBuff, BUFF2_MAX, "%d", Pm25);
  u8g.drawStr( 0, 39, "PM25 :");

  memset(tBuff,0, BUFF2_MAX);
  snprintf(tBuff, BUFF2_MAX, "%d", Pm25);
  u8g.setFont(u8g_font_fub11n);
  u8g.drawStr( 60, 39, tBuff);

  u8g.setFont(u8g_font_9x18r);
  memset(tBuff,0, BUFF2_MAX);
  u8g.drawStr( 0, 64, "PM10 :");
  
  memset(tBuff,0, BUFF2_MAX);
  snprintf(tBuff, BUFF2_MAX, "%d", Pm10);
  u8g.setFont(u8g_font_fub11n);
  u8g.drawStr( 60, 64, tBuff);
  
#ifdef DEBUG
  //Serial.println(tBuff);
#endif
  
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //init realtime clock
  DS3231_init(DS3231_INTCN);

  pinMode(OLED_RESET, OUTPUT);           
  digitalWrite(OLED_RESET, HIGH);  

  // SoftwareSerial I/F setup such as SDS011 dust sensor
  SDS011_Serial.begin(9600);

  //Pm25 : 1um, 0~999g/m^3, Pm10: 25um, 0~999g/m^3    
  Pm25=0;
  Pm10=0;
  
  // flip screen, if required
  // u8g.setRot180();
  
  // set SPI backup if required
  //u8g.setHardwareBackup(u8g_backup_avr_spi);

  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }
  //set font
  u8g.setFont(u8g_font_unifont);
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned short tempUpdateFlag = 0;
  unsigned long now = millis();
  char tBuff[BUFF_MAX*2] = {0,};
  uint8_t temp_msb, temp_lsb;

  if (now - prev > interval) {

     DS3231_get(&(g_tData.current_time));
     ProcessSerialData();
#ifdef DEBUG     
     serialDebugDisp();
#endif
     //byte temp_msb = DS3231_get_MSB();
     //byte temp_lsb = DS3231_get_LSB();

      //sprintf(tBuff,"%d, %d", temp_msb, temp_lsb);
      //sprintf(tBuff,"%d, %d", temp_msb, 0);
#if 0
     if ( tempUpdateFlag == 0 )
     {

        if (TEMP_UPDATE_INTERVAL > tempUpdateFlag) {
          tempUpdateFlag ++;
        } else {
          tempUpdateFlag = 0;
        }
     }
#endif
     //print_time_to_lcd(&(g_tData.current_time), 0);
  
    u8g.firstPage();  
    do {
      draw(&(g_tData.current_time), 0);
    } while( u8g.nextPage() );
      
    prev = now;

  }
  
  delay(250);
}
