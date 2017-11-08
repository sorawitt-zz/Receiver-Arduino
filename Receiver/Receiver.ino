#include <SPI.h>


////////////////////// PPM CONFIGURATION//////////////////////////
#define channel_number 6  //set the number of channels
#define sigPin 2  //set PPM signal output pin on the arduino
#define PPM_FrLen 27000  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 400  //set the pulse length

int ppm[channel_number];

/* NOT USE
struct RCData {
  long throttle;
  long yaw;
  long pitch;
  long roll;
  long AUX1;
  long AUX2;
};
RCData outData;
*/
String inData;


unsigned long timer;
unsigned long INTERVAL = 100;

void readInput() {
  while (Serial.available() > 0)
  {
    char received = Serial.read();
    inData += received;

    // Process message when new line character is recieved
    if (received == '\n')
    {
      processInput(inData);
      inData = ""; // Clear recieved buffer
    }
  }
}

void processInput(String line) {
 // Serial.print("msg: ");
  //Serial.print(line);
  const int numberOfPieces = 6;
  String pieces[numberOfPieces];
  int counter = 0;
  int lastIndex = 0;

  for (int i = 0; i < line.length(); i++) {
    // Loop through each character and check if it's a comma
    if (line.substring(i, i + 1) == ",") {
      // Grab the piece from the last index up to the current position and store it
      pieces[counter] = line.substring(lastIndex, i);
      // Update the last position and add 1, so it starts from the next character
      lastIndex = i + 1;
      // Increase the position in the array that we store into
      counter++;
    }

    // If we're at the end of the string (no more commas to stop us)
    if (i == line.length() - 1) {
      // Grab the last part of the string from the lastIndex to the end
      pieces[counter] = line.substring(lastIndex, i);
    }
  }
  /*
    outData.throttle = pieces[0].toInt();
    outData.yaw      = pieces[1].toInt();
    outData.pitch    = pieces[2].toInt();
    outData.roll     = pieces[3].toInt();
    outData.AUX1     = pieces[4].toInt();
    outData.AUX2     = pieces[5].toInt();
  */

  ppm[0] = pieces[0].toInt();
  ppm[1] = pieces[1].toInt();
  ppm[2] = pieces[2].toInt();
  ppm[3] = pieces[3].toInt();
  ppm[4] = pieces[4].toInt();
  ppm[5] = pieces[5].toInt();
}

void setupPPM() {
  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, 0);  //set the PPM signal pin to the default state (off)

  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;

  OCR1A = 100;  // compare match register (not very important, sets the timeout for the first interrupt)
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei();
}

void setup() {
  setupPPM();
  Serial.begin(115200);
  timer = millis();
}


void output() {
  Serial.print("Throttle: "); Serial.print(ppm[0]);       Serial.print("    ");
  Serial.print("Yaw: ");      Serial.print(ppm[1]);       Serial.print("    ");
  Serial.print("Pitch: ");    Serial.print(ppm[2]);       Serial.print("    ");
  Serial.print("Roll: ");     Serial.print(ppm[3]);       Serial.print("    ");
  Serial.print("Aux1: ");     Serial.print(ppm[4]);       Serial.print("    ");
  Serial.print("Aux2: ");     Serial.print(ppm[5]);       Serial.print("\n");
}

void loop() {
  readInput();
  if ((millis() - timer) > INTERVAL) {
    output();
    timer = timer + INTERVAL;
  }
}


#define clockMultiplier 1 // set this to 2 if you are using a 16MHz arduino, leave as 1 for an 8MHz arduino

ISR(TIMER1_COMPA_vect) {
  static boolean state = true;

  TCNT1 = 0;

  if ( state ) {
    //end pulse
    PORTD = PORTD & ~B00000100; // turn pin 2 off. Could also use: digitalWrite(sigPin,0)
    OCR1A = PPM_PulseLen * clockMultiplier;
    state = false;
  }
  else {
    //start pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;

    PORTD = PORTD | B00000100; // turn pin 2 on. Could also use: digitalWrite(sigPin,1)
    state = true;

    if (cur_chan_numb >= channel_number) {
      cur_chan_numb = 0;
      calc_rest += PPM_PulseLen;
      OCR1A = (PPM_FrLen - calc_rest) * clockMultiplier;
      calc_rest = 0;
    }
    else {
      OCR1A = (ppm[cur_chan_numb] - PPM_PulseLen) * clockMultiplier;
      calc_rest += ppm[cur_chan_numb];
      cur_chan_numb++;
    }
  }
}
