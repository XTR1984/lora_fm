#include <Arduino.h>
#include <SX127x.h>
#include "SX127x_register.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

QueueHandle_t interruptQueue;
#define VERSION "1.0.0"

//#define FREQUENCY 452925000
#define DEFAULTFREQUENCY 432800000
#define DEFAULTRATE 8000

#define LED 15 

uint64_t freq=DEFAULTFREQUENCY;
int      rate = DEFAULTRATE;

#define BUFFERSIZE 8000
#define MODE_OFF 0
#define MODE_RX 1
#define MODE_TX 2

bool led_state = true;
int mode = MODE_OFF;
int txsize = 0;
int cur_txsize = 0;
hw_timer_t *timer = NULL;


uint8_t buffer1[BUFFERSIZE];
uint8_t buffer2[BUFFERSIZE];
uint8_t *curbuffer = buffer1;
uint8_t *prevbuffer = buffer2;
char pcm1,pcm2;
int bindex=0;

void timerstop(){
  timerStop(timer);
  led_state =false;
  digitalWrite(LED,led_state);
}


void ARDUINO_ISR_ATTR onTimer(){
  if (mode==MODE_RX){
    pcm1 = SX127x_read_reg(REG_FEI_MSB);
    pcm2 =  SX127x_read_reg(REG_FEI_LSB) ; 
    //uint16_t pcm = pcm1<<8 + pcm2;
    curbuffer[bindex++] = pcm2 + 127;    //8bit sampling
  }  
  if (mode==MODE_TX){
    pcm2 = curbuffer[bindex++]-127;
    SX127x_write_reg(REG_FDEV_LSB, pcm2);
    if (bindex>cur_txsize+5){
       mode=MODE_OFF;
       SX127x_standby();
       timerstop();
    }
  }
  if (bindex==BUFFERSIZE ){
    bindex = 0;
    char msg = 1;
    uint8_t *tbuf = curbuffer;
    curbuffer = prevbuffer;
    prevbuffer = tbuf;   
    cur_txsize = txsize;
    xQueueSendFromISR(interruptQueue, &msg, NULL);
  }

}



void timerstart(){
  // Set timer frequency 
  timer = timerBegin(1440000);
  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer);
  // Set alarm to call onTimer function every 125 microseconds.
  // Repeat the alarm (third parameter) with unlimited count = 0 (fourth parameter).
  timerAlarm(timer, 1440000/rate, true, 0);   
  led_state =true;
  digitalWrite(LED,led_state);
}


void setup() {
  setCpuFrequencyMhz(240);
  pinMode(LED,OUTPUT);
  digitalWrite(LED, LOW);  
  Serial.begin(921600);
  Serial.setRxBufferSize(BUFFERSIZE);
  //Serial.setTimeout(1);
  Serial.println("lora fm  fw " VERSION);
  //Serial.setTxTimeoutMs()
  delay(1000);
  interruptQueue = xQueueCreate( 100, 1 );
  SX127x_begin();
  SX127x_standby();

}


String getValue(String data, char separator, int index)  //by Odis Harkins https://arduino.stackexchange.com/a/1237
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


void commandprocessor(){
  String str = Serial.readString();
  if(str.startsWith("init")){
    SX127x_begin();
    char vers = SX127x_read_reg(0x42);
    if (vers!=0x12)
    {
      Serial.println("fail read version");
    }
    else {  
      int freq1 = getValue(str,':',1).toInt();          
      freq = (uint64_t) freq1;
      SX127x_set_frequency(&freq);       
      rate = getValue(str,':',2).toInt();          
    
      SX127x_set_TX_power(2, false);    // минимальная мощность
      // set bitrate from 4.8kb/s to 48кb/s.
      SX127x_write_reg(REG_BITRATE_MSB, 0x2);
      SX127x_write_reg(REG_BITRATE_LSB, 0x9B);
      //SX127x_write_reg(REG_PLL_HOP, 0xAD);   // freq hop mode 
      
      //fdev = fdev/61;
      //SX127x_write_reg(0x4, fdev>>8);
      //SX127x_write_reg(0x5, fdev&&0xff);
      Serial.println("ok");
    }
    //Serial.print(str);
    bindex = 0;
  }  
  if(str.startsWith("setfreq")){
    SX127x_standby();
    int freq1 = getValue(str,':',1).toInt();          
    freq = (uint64_t) freq1;
    SX127x_set_frequency(&freq);       
    Serial.print("ok\r\n");
  }  

  if (str.startsWith("read")){
     SX127x_standby();      
     SX127x_write_reg(REG_DETECT_OPTIMIZE, 0x00);   // continuous MODE
     SX127x_write_reg(REG_OP_MODE, 0x05);   //rx MODE
     mode=MODE_RX; 
     timerstart();
  }
  if (str.startsWith("write")){
      SX127x_standby();      
      int pwr = getValue(str,':',1).toInt();          
      if(pwr <= 15) SX127x_set_TX_power(pwr, false); // For PWR between 2-15: Do not use the +20dBm option on PA_BOOT
      else SX127x_set_TX_power(pwr, true); // For PWR between 5-20: Enable the +20dBm option on PA_BOOT
      Serial.write("ready\r\n");
      Serial.flush();
      while(!Serial.available()) {
        led_state = !led_state;
        digitalWrite(LED,led_state);
        delay(50);
      };
      cur_txsize = Serial.read(curbuffer, BUFFERSIZE);
      //Serial.print("curtxsize= ");
      //Serial.println(cur_txsize);
      if (cur_txsize>0 ){
        SX127x_write_reg(REG_DETECT_OPTIMIZE, 0x00);   // continuous MODE
        SX127x_write_reg(REG_OP_MODE, 0x0B);   //tx MODE
        mode=MODE_TX; 
        timerstart();
      }  

   }

  if(str.startsWith("stop")){
    timerstop();
    SX127x_standby();      
    led_state =false;
    digitalWrite(LED,led_state);
    Serial.flush();
    mode=MODE_OFF;
  }

}

void loop() {
  if (Serial.available() > 0) {
    if(mode!=MODE_TX) commandprocessor();  
  }  
  if (mode==MODE_RX){
      char msg;
      if(xQueueReceive(interruptQueue, &msg, portMAX_DELAY)) {
        led_state =!led_state;
        digitalWrite(LED,led_state);
        //Serial.print((char)pcm);
        Serial.write(prevbuffer, BUFFERSIZE);
        Serial.flush();
      }  
  }  
  if (mode==MODE_TX){
    char msg;
    Serial.write("ready\r\n");
    Serial.flush();
    led_state =!led_state;
    digitalWrite(LED,led_state);
    int wc=20;
    while(!Serial.available() &&wc>0){
       delay(50);
       wc--;
    };
    if(wc==0){
      mode=MODE_OFF;
      timerstop();
      SX127x_standby();
      bindex=0;
    }
    txsize = Serial.read(prevbuffer, BUFFERSIZE);
    //Serial.print("txsize= ");
    //Serial.println(txsize);
    xQueueReceive(interruptQueue, &msg, pdMS_TO_TICKS(1000));  
  }  

}


