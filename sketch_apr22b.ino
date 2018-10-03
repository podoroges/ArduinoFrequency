#include <TFT.h>  // Arduino LCD library
#include <SPI.h>
#include <Encoder.h>
//
#include <Wire.h>
#include <Adafruit_SI5351.h>
Adafruit_SI5351 clockgen = Adafruit_SI5351();


// Prepare rotary encoder
Encoder myEnc(2, 3);
long oldPosition  = -999;

// Prepare display
#define cs   10
#define dc   9
#define rst  8
TFT tft = TFT(cs, dc, rst);

// Tick arrays
#define len 20
int lo[len]; // Long old
int so[len]; // Short old
int ln[len]; // Long new
int sn[len]; // Short new
int lol = 0;
int sol = 0;
int lnl;
int snl;

//
#define scale_offset 50 // Shift scale vertical
#define scale_short  10 // Height of short tick
#define scale_long   20 // Height of long tick

uint16_t col = 0xFFFF; // Drawing color
float f = 3; // Working frequency
float zoom = 5000.0; // how wide our scale
int hw; //halfwidth - for faster calculations

void setup(void) {
  Serial.begin(9600);
  clockgen.begin();
 
  tft.begin();
  hw = tft.width()/2;
  tft.fillScreen(ST7735_BLACK); 
  tft.setTextColor(col,ST7735_BLACK);
  tft.setTextWrap(false);
}

// Frequency to pixel conversion
int w(float _f){
  return hw+(_f-f)*zoom;
}


// Draw Scale Tick and remember its location 
void AddScaleTick(int x,int h){
  if(x==hw)
    return; // Draw nothing at center line
  if(h==scale_long){
    ln[lnl] = x;
    if(lnl<len)
      lnl++;     
  }
  else{
    sn[snl] = x;
    if(snl<len)
      snl++;
  }
  tft.drawFastVLine(x, scale_offset, h, col); 
}

// Draw screen with given freq and color
float fo;
void draw1(float f1,uint16_t color){
  if(fo==f)
    return;
  fo = f;  
  tft.setTextSize(2);

  float f0 = int(f1*100)/100.0;
  int w0 = w(f0);
  // Go left till screen end
  do{
    w0 = w0-zoom*0.01;
    f0 = f0-0.01;
  }while(w0>0);

  // Clear new arrays
  lnl = 0;
  snl = 0;

  // Redraw text
  char ca[10];
  int rx = 0; // black rect x


 // tft.fillRect(0,scale_offset+scale_long+19,hw*2,4,ST7735_BLACK);
  for (int16_t x=w0; x < tft.width()+12*4; x+=zoom*0.01) {
    f0+=0.01;
    if((x>-20)&&(x<tft.width()+20)&&(!(int(f0*100)%2))){
      int tx = x-20;
      if(tx>0){
        int bw = tx-rx;
        if(tx>tft.width())
          bw = tft.width()-rx;
        if((rx<hw)&&(rx+bw>hw)){
          tft.fillRect(rx,scale_offset+scale_long+5,hw-rx,14,ST7735_BLACK);
         // tft.drawRect(rx,scale_offset+scale_long+19,hw-rx,4,col);
          tft.fillRect(hw+1,scale_offset+scale_long+5,bw-(hw-rx),14,ST7735_BLACK);
        //  tft.drawRect(hw+1,scale_offset+scale_long+19,bw-(hw-rx),4,col);
        }
        else{
          tft.fillRect(rx,scale_offset+scale_long+5,bw,14,ST7735_BLACK);
        }

      }  
      
      
      tft.setCursor(tx, scale_offset+scale_long+5);  
      dtostrf(f0,1,2,ca);  
      rx = tx+12*strlen(ca);  
      //tft.drawRect(x-20, scale_offset+scale_long+5,12*strlen(ca),14,col);
   
      tft.print(ca);
      //if(tx<0)// Eliminate right side bug
      //  tft.drawFastVLine(tft.width()-1, scale_offset+scale_long+5,14, col);
    }    
  }
  if(rx<tft.width()){
    tft.fillRect(rx,scale_offset+scale_long+5,tft.width()-rx,14,ST7735_BLACK);
    //tft.drawRect(rx,scale_offset+scale_long+19,tft.width()-rx,4,ST7735_BLUE);
  }
  // Calculating loop
  for (int16_t x=w0; x < tft.width(); x+=zoom*0.01) {
    // Long tick
    if(x>0){
      AddScaleTick(x,scale_long);
      if(x+1 < tft.width())
        AddScaleTick(x+1,scale_long);
    }
    // Short tick  
    float x1 = x+zoom*0.005;  
    if((x1 < tft.width())&&(x1>0)){
      AddScaleTick(x1,scale_short);
      if(x1+1 < tft.width())
        AddScaleTick(x1+1,scale_short);
    }
  }

  // Erasing long old
  for(int a=0;a<lol;a++){
    int x = lo[a];
    // Checking with long new
    int found = -1; 
    for(int i=0;i<lnl;i++)
      if(ln[i]==x)
        found = i;
    if(found<0){
      // Nothing in old found, now check short
      for(int i=0;i<snl;i++)
        if(sn[i]==x)
          found = i;
      if(found>=0){
        // Short new in this place, erasing tail...
        tft.drawFastVLine(x, scale_offset+scale_short,scale_long-scale_short, ST7735_BLACK);
      }
      else{
        // Nothing in this place, erase completely
        tft.drawFastVLine(x, scale_offset, scale_long, ST7735_BLACK);
      }
    }  
  }
  // Erasing short old
  for(int a=0;a<sol;a++){
    int x = so[a];
    // Checking with long new
    int found = -1; 
    for(int i=0;i<lnl;i++)
      if(ln[i]==x)
        found = i;
    if(found<0){
      // Nothing in old found, now check short
      for(int i=0;i<snl;i++)
        if(sn[i]==x)
          found = i;
      if(found>=0){
        // Short new in this place, nothing to do
      }
      else{
        // Nothing in this place, erase completely
        tft.drawFastVLine(x, scale_offset,scale_short, ST7735_BLACK);
      }
    }  
  }
  // Copy arrays new -> old
  for(int a=0;a<lnl;a++)
    lo[a] = ln[a];
  lol = lnl;  
  for(int a=0;a<snl;a++)
    so[a] = sn[a];
  sol = snl;   

  // Center line
  tft.drawFastVLine(hw, 40, 50, ST7735_BLUE);
}

// Draw big freq text
void draw2(float f1){
  tft.setTextSize(2);
  char ca[10];
  // We need to insert dot before last digit. Thats why we make precision 5 iso 4 
  dtostrf(f1,1,5,ca);
  // Get lenght of obtained char array  
  int ln = strlen(ca);
  // Put prelast char to the last position
  ca[ln-1] = ca[ln-2];
  // Pun dot into plelast position
  ca[ln-2] = '.';
  tft.setCursor(hw-40, 20);    
  tft.print(ca);
}

void setgen(float f1){

  if(f1<3){
    long fi = f1*200000;
    long n = fi%12500;
    long m = (fi-n)/12500;  
    clockgen.setupPLL(SI5351_PLL_B, m, n, 12500);
    clockgen.setupMultisynth(2, SI5351_PLL_B, 400, 0, 1);
    clockgen.setupRdiv(2, SI5351_R_DIV_1);
    Serial.print(" f1=");
    Serial.print(f1);
    Serial.print(" m=");
    Serial.print(m);
    Serial.print(" n=");
    Serial.print(n);
    Serial.print(" f5=");
    float f5 = 25000/400*((float)m+(float)n/12500);
    Serial.println(f5);  
  }
  
  if(f1>=3 && f1<=6){
    long fi = f1*100000;
    long n = fi%12500;
    long m = (fi-n)/12500;  
    clockgen.setupPLL(SI5351_PLL_B, m, n, 12500);
    clockgen.setupMultisynth(2, SI5351_PLL_B, 200, 0, 1);
    clockgen.setupRdiv(2, SI5351_R_DIV_1);
    Serial.print(" f1=");
    Serial.print(f1);
    Serial.print(" m=");
    Serial.print(m);
    Serial.print(" n=");
    Serial.print(n);
    Serial.print(" f5=");
    float f5 = 25000/200*((float)m+(float)n/12500);
    Serial.println(f5);  
  }

  if(f1>6 && f1<=11){
    long fi = f1*500000;
    long n = fi%125000;
    long m = (fi-n)/125000;  
    clockgen.setupPLL(SI5351_PLL_B, m, n, 125000);
    clockgen.setupMultisynth(2, SI5351_PLL_B, 100, 0, 1);
    clockgen.setupRdiv(2, SI5351_R_DIV_1);
    Serial.print(" f1=");
    Serial.print(f1);
    Serial.print(" m=");
    Serial.print(m);
    Serial.print(" n=");
    Serial.print(n);
    Serial.print(" f5=");
    float f5 = 25000/100*((float)m+(float)n/125000);
    Serial.println(f5);  
  }


  // todo
  if(f1>11 && f1<=100){
    long fi = f1*500000;
    long n = fi%125000;
    long m = (fi-n)/125000;  
    clockgen.setupPLL(SI5351_PLL_B, m, n, 125000);
    clockgen.setupMultisynth(2, SI5351_PLL_B, 100, 0, 1);
    clockgen.setupRdiv(2, SI5351_R_DIV_1);
    Serial.print(" f1=");
    Serial.print(f1);
    Serial.print(" m=");
    Serial.print(m);
    Serial.print(" n=");
    Serial.print(n);
    Serial.print(" f5=");
    float f5 = 25000/100*((float)m+(float)n/125000);
    Serial.println(f5);   
  }

  
  clockgen.enableOutputs(true);    
}

void loop() {
  draw1(f,col);
  long newPosition = myEnc.read();
  if (newPosition != oldPosition) {
    // Calculate new frequency
    f  =  f+0.001*(oldPosition - newPosition);
    // Draw freq screen
    draw1(f,col);
    // Draw big freq text
    draw2(f);    
    // Set Generator
    setgen(f);
    oldPosition = newPosition;
  }
}

