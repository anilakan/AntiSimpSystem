/*
 * Anti-Simp System
 * by Anisha Nilakantan (anilakan)
 * 
 * This code takes in a "sent" and "received" signal to measure how long it 
 *  took someone to respond to the user, stores it in an array of the last
 *  five responses, calulates an average, then cases on the user's pettiness
 *  levels to set a timer for how long the user must wait to respond, changing
 *  the LED colors to indicate what stage of waiting they are in. While the 
 *  timer is set, the user may not touch their phone, else risk setting off
 *  the break beam sensors and getting a randomized mean message displayed on
 *  the LCD screen, as well as the LEDs going red while the break beam sensor
 *  senses the user's hand in the box. 
 *  
 *  
 *  Some code based on these sources:
 *      LED: Arduino Examples for Pololu LED Strip
 *      IR transmitter and receiver: https: //create.arduino.cc/projecthub/electr
 *                                   opeak/use-an-ir-remote-transmitter-and-receiv
 *                                   er-with-arduino-1e6bc8
 *                                   
  Pin Table:
  pin     |     mode    |   description
  --------------------------------------
  4           input         break beam sensor
  5           output        d4 LCD
  6           output        d5 LCD
  7           output        d6 LCD
  8           output        d7 LCD
  10          output        IR receiver
  11          output        EN LCD
  12          output        LED strip
  13          output        RS LCD

*/

#include <IRremote.h>
#define RECEIVER 10
#define Send_Button 0xFF02FD // you send a message (play/pause)
#define Receive_Button 0xFFE21D // you receive a message (func/stop button)
#define Odr 0xFFC23D // you're left on read (skip forward button)

// 3 pettiness levels
#define Button_1 0xFF30CF 
#define Button_2 0xFF18E7
#define Button_3 0xFF7A85
#define Button_4 0xFF10EF
#define Button_5 0xFF38C7

unsigned long send_time;
unsigned long response_time[] = {0, 0, 0, 0, 0};
unsigned long res_average;
unsigned long wait_time;
unsigned long response_time_stamp;
unsigned long petty_wait;
unsigned long petty_start;
unsigned long start;
unsigned long wait;
int case5p1 = 0;
int petty = 1; // default is level 1 pettiness
int index_r_time; 
int current_i;
uint32_t Previous;
IRrecv irrecv(RECEIVER);
decode_results results;

#include <PololuLedStrip.h>
PololuLedStrip<12> ledStrip;
#define LED_COUNT 57
rgb_color colors[LED_COUNT];
#define BEAMPIN 4
int beamState = 0;
int beamLastState = 0;
int on_or_off = 0;
int redState = 0;

#include <Wire.h>
#include <LiquidCrystal.h>
const int d4 = 5, d5 = 6, d6 = 7, d7 = 8, rs = 13, en = 11;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
String meanMessages[] = {"Stop simping    ", "                ", "You can do so mu", "ch better       ", "Put. It. Down.  ", "                ",
                         "Not very hot gir", "l summer :(     ", "Have some self r", "espect.         ", "You look so desp", "erate rn.       ",
                         "He's literally u", "gly stop        ", "He's not thinkin", "g about you     ", "He looks like he", "can't swim.     ",
                         "He was hotter   ", "online          "
                        }; // each message is two lines (top then bottom to be printed)


// Helper Functions: 

// writes to the top line of the LCD
void lcdWrite(String s) {
  lcd.setCursor(0, 0);
  lcd.print(s);
}

// writes to the bottom line of the LCD
void lcdWriteBottom(String s) {
  lcd.setCursor(0, 1);
  lcd.print(s);
}

//sets the LED color for the entire strip
void LEDColorSet(int R, int G, int B) {
  rgb_color color;
  color.red = R;
  color.green = G;
  color.blue = B;

  for (uint16_t i = 0; i < LED_COUNT; i++) {
    colors[i] = color;
  }
  ledStrip.write(colors, LED_COUNT);

  Serial.print("Showing color: ");
  Serial.print(color.red);
  Serial.print(",");
  Serial.print(color.green);
  Serial.print(",");
  Serial.println(color.blue);
}

//display
void LCDMeanMessage() {
  // randomize a number
  // select that from an array of strings
  // print that to the LCD
  int index = random(0, 999); // randomizing between 0 to 999 and then mapping to 0-9
  index = index / 100;         

  Serial.println(index, DEC);
  lcd.setCursor(0, 0);
  lcdWrite(meanMessages[index * 2]); // prints top line of message
  lcdWriteBottom(meanMessages[index * 2 + 1]); // prints bottom line of message
  delay(2000); 
}

// calculates how long the user should wait based on the last 5 response times (stored in array)
//  and pettiness level
unsigned long how_long_to_wait(unsigned long response_time[], int petty) {
  // taking the average of the response time, can make it more petty later
  // can add in function from button to gauge how petty you're feeling, take in this parameter to go to one of three cases
  unsigned long sum = 0;
  for (int j = 0; j < 5; j++) {
    sum += response_time[j];
  }
  unsigned long average = sum / 5;
  Serial.print("Petty: ");
  Serial.println(petty, DEC);

  wait_time = 0;
  switch (petty) {
    case 1:
      wait_time = 20000; // reply when you want
      break;
    case 2:
      wait_time = 300000; // wait five minutes
      break;
    case 3:
      wait_time = average; // wait the same time they have
      break;
    case 4:
      wait_time = average * 2; // wait double the time
      break;
    case 5:
      wait_time = average * 2.5; //wait 2.5x the time
      // set on_or_off off??
      case5p1 = 1;
      break;
    default:
      wait_time = 10000;
      break;
  }
  res_average = average;
  return wait_time;
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  irrecv.enableIRIn();
  pinMode(BEAMPIN, INPUT);
  digitalWrite(BEAMPIN, HIGH);
}

void loop() {

  // take in what button is being pressed
  if (irrecv.decode(&results)) {

    Serial.println(results.value, HEX);


    switch (results.value) {
      case Send_Button:
        send_time = millis();

        lcdWrite("Keep him waiting");
        lcdWriteBottom("      ;)        ");
        LEDColorSet(0, 102, 102); 
        break;
      case Receive_Button:
        index_r_time++; // increase r-time by 1
        current_i = index_r_time % 5;
        response_time_stamp = millis();
        response_time[current_i] = millis() - send_time;
        //Serial.print("Response time: "); Serial.println(response_time[current_i], DEC);
        LEDColorSet(0xFF, 0x73, 0x00);
        wait_time = how_long_to_wait(response_time, petty);
        //Serial.print("Wait time "); Serial.println(wait_time, DEC);
        lcdWrite("Keep him waiting");
        lcdWriteBottom("      ;)        ");
        on_or_off = 1;
        break;
      case Odr: // ODR stands for "Opened, Didn't Respond."
        LEDColorSet(255, 0, 0);
        lcdWrite("DUMP. HIS. ASS. ");
        lcdWriteBottom("                ");
        // still runs the loop right now
        break;
      case Button_1:
        petty = 1;
        break;
      case Button_2:
        petty = 2;
        break;
      case Button_3:
        petty = 3;
        break;
      case Button_4:
        petty = 4;
        break;
      case Button_5:
        petty = 5;
        break;
      default:
        break;
    }
    irrecv.resume();
  }

  
  beamState = digitalRead(BEAMPIN);

  // if the timer is running, then this code runs
  if (on_or_off) {

    // beam has been broken, hand is in the box
    if (beamState == LOW && millis() - response_time_stamp < wait_time) {
      LEDColorSet(255, 0, 0); // eventually make this a flash?
      LCDMeanMessage();
      redState = 1;
      lcd.clear(); // want to make messages stay on for longer !
    }
    // return back to orange (the waiting time color)
    if ((redState == 1) && (beamState == HIGH)) {
      LEDColorSet(0xFF, 0x73, 0X00);
      redState = 0;
    }
    // turns off the timer if the correct time has elapsed
    if (millis() - response_time_stamp >= wait_time) {
      Serial.print("different "); Serial.println(millis() - response_time_stamp, DEC);

      LEDColorSet(0, 255, 00);
      send_time = 0;
      lcdWrite("U can now reply!");
      lcdWriteBottom("                ");
      if (petty = 5) {
        case5p1 = 0;   // note that you can't really change the value in the middle
      }
      Serial.print("Send time: "); Serial.println(send_time, DEC);
      redState = 0;
      on_or_off = 0;

      // response time is time since send was hit to response was hit
      // if elapsed time SINCE THEN is greater than wait time
      // millis() - response_time_stamp which is the actual millis() it was hit
    }
  }

  // Level 5 pettiness instructions
  switch (petty) {
    case 5:
      // set the LED to purple
      if (case5p1 == 1) {
        LEDColorSet(128, 128, 0);
        // wait for petty_wait / 2
        petty_start = millis();
        petty_wait = res_average / 2; 
        if (millis() - petty_start > petty_wait) {
          LEDColorSet(0, 0, 128);
          lcdWrite("Post on your sto");
          lcdWriteBottom("ry!             ");
          case5p1 = 2;
        }
      }
      if (case5p1 == 2) {
        start = millis();
        wait = res_average / 5;
        if (millis() - start > wait) {
          case5p1 = 3;
        }
      }
      if (case5p1 == 3) {
        petty_wait = res_average * 3 / 4;
        petty_start = millis();
        if (millis() - petty_start > petty_wait) {
          LEDColorSet(230, 0, 126);
          lcdWrite("View their story");
          lcdWriteBottom(", don't respond."); 
        }
        case5p1 = 4;
      }
      if (case5p1 == 3) {
        start = millis();
        wait = res_average / 5;
        if (millis() - start > wait) {
          case5p1 = 5;
        }
      }
      break;
    default:
      break;
  }
}
