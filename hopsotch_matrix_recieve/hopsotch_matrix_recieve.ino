#include <stdlib.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <FastLED.h>

/* Basic Artnet code is from this post. There is a libray someone put together for artnet on teeny but it was very incosistant.
   This runs at 60fps without dropping any frames
   https://forum.pjrc.com/threads/24688-Artnet-to-OctoWS2811?p=42720&viewfull=1#post42720
   see github readme and images for more info 
*/

#define row_num 1 // sets this row's number. Automatically sets the ip and mac address
#define printer 0 //prints warnings of how many packets were dropped
byte test_mode = 0; // displays a patternt to make sure the rows are in the correct order

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0x4C, row_num + 10} ; //the mac adress in HEX of ethernet module/shield

byte ip[] = { 2, 0, 0, row_num + 10}; //this unit/s ip. the first one will be 2.0.0.11.
//The network adapter of the device sending artnet should be 2.0.0.1

#define color_order RBG  //color order of LEDs used

const int ledsPerStrip = 13; //how many LED elemnts on each individual strip
const int stripCount = 14; //number of strips


unsigned int localPort = 6454;      // DO NOT CHANGE artnet UDP port is by default 6454
#define bytes_to_short(h,l) ( ((h << 8) & 0xff00) | (l & 0x00FF) );
DMAMEM int displayMemory[ledsPerStrip * stripCount];
int drawingMemory[ledsPerStrip * stripCount];
byte pins[stripCount] = {0, 1, 2, 3, 4, 5, 6, 7, 23, 22, 21, 20, 19, 18};
CRGB strip[stripCount][ledsPerStrip];
uint32_t cm, prev[9];

const int first_universe_number = 0;//CHANGE FOR YOUR SETUP most software this is 1, some software send out artnet first universe as zero.
// some software sends 0 as omni so it's probably best to keep this at 0 and sent 1 as your first row

const int universeSize = ledsPerStrip * stripCount;  //how many leds in the univers, not the amount of RGB elemnts
const int number_of_channels = universeSize * 3; //will be 3*universeSize. total number of r led elements
byte channel_buffer[number_of_channels * 4]; // "buffer to store filetered DMX data//SHOULD BE SAME AS number_of_channels" I increasded this to make sure there wasn't going to be any issues. Most prob not necessary
//byte buff2[number_of_channels * universeSize]; // increase buffer for filtered data to cover size of your total array(removed art-net header)
byte buff2[number_of_channels * 2 * 8]; // increase buffer for filtered data to cover size of your total array(removed art-net header)

const int MAX_BUFFER_UDP = 768;//leave as is
char packetBuffer[MAX_BUFFER_UDP]; //buffer to store incoming data
short incoming_universe = 0; //leave as is (if suspect uni number issues, try changing first_uni number above first.
const int start_address = 0; // 0 if you want to read from channel 1
const int art_net_header_size = 17; //dnt change
byte prev_seq, sequence;

float max_bright = 1; //bringess of artnet data recieved 0.0-1.0
float lfo_bright = .2; //brightness in test mode

EthernetUDP Udp;
int strand_count, ball_count;
int lfo[4], latch[4];
byte order[14] = {7, 8, 9, 10, 11, 12, 13 , 0, 1, 2, 3, 4, 5, 6};


void setup() {
  int start_dly = 250;
  //Serial.begin(115200);
  delay(100);
  Serial.println("hi");

  // setup all the strips
  // <elemnet type, pin, color order>
  FastLED.addLeds<WS2811, 0, color_order>(strip[6], ledsPerStrip);
  FastLED.addLeds<WS2811, 1, color_order>(strip[5], ledsPerStrip);
  FastLED.addLeds<WS2811, 2, color_order>(strip[4], ledsPerStrip);
  FastLED.addLeds<WS2811, 3, color_order>(strip[3], ledsPerStrip);
  FastLED.addLeds<WS2811, 4, color_order>(strip[2], ledsPerStrip);
  FastLED.addLeds<WS2811, 5, color_order>(strip[1], ledsPerStrip);
  FastLED.addLeds<WS2811, 6, color_order>(strip[0], ledsPerStrip);

  FastLED.addLeds<WS2811, 17 , color_order>(strip[13], ledsPerStrip);
  FastLED.addLeds<WS2811, 18, color_order>(strip[12], ledsPerStrip);
  FastLED.addLeds<WS2811, 19, color_order>(strip[11], ledsPerStrip);
  FastLED.addLeds<WS2811, 20, color_order>(strip[10], ledsPerStrip);
  FastLED.addLeds<WS2811, 21, color_order>(strip[9], ledsPerStrip);
  FastLED.addLeds<WS2811, 22, color_order>(strip[8], ledsPerStrip);
  FastLED.addLeds<WS2811, 23, color_order>(strip[7], ledsPerStrip);

  pinMode(8, INPUT_PULLUP);
  delay(500);
  if ( butt == 0 ) {
    test_mode = 1; //hold the button while turing on to enter test mode
  }

  delay(row_num * start_dly); //used to stagger the turn on of each row

  Ethernet.begin(mac, ip);
  delay(250);
  Udp.begin(localPort);
  delay(250);
  Serial.print("test mode: ");
  Serial.println(test_mode);

  Serial.println("done");

}

void loop() {
  uint32_t ct = micros();
  uint32_t cm = millis();

  int packetSize = 0;
  if (test_mode == 0) {
    packetSize = Udp.parsePacket();
  }

  if (packetSize || test_mode == 1)
  {
    Udp.read(packetBuffer, MAX_BUFFER_UDP);
    incoming_universe = bytes_to_short(packetBuffer[15], packetBuffer[14]);

    if (printer == 1) {
      prev_seq = sequence;
      sequence = packetBuffer[12];

      if (prev_seq != sequence - 1) {  //"dropped only woks when every packet is read. if we ignore universes then we should see constant drops
        if (prev_seq != 255 && sequence != 0) {
          Serial.print("!!                         DROPED ");
          Serial.println(sequence - prev_seq);
        }
      }
    }

    if (incoming_universe == 1 || test_mode == 1) {

      for (int i = start_address; i < number_of_channels; i++) {
        channel_buffer[i - start_address] = byte(packetBuffer[i + art_net_header_size + 1]);
      }

      for (int i = 0; i < number_of_channels; i++) {
        buff2[i + ((incoming_universe - first_universe_number)*number_of_channels)] = channel_buffer[i - start_address];
      }

      int sel = 0;
      int cnt1 = 0;
      for (int i = 0; i < 7; i++) {
        for (int j = 0; j < ledsPerStrip; j++) {
          sel = cnt1 + (number_of_channels);
          //strip[i][j] = CRGB( lfo, 0, 0);
          strip[i][j] = CRGB( buff2[sel] * max_bright, buff2[sel + 1] * max_bright, buff2[sel + 2] * max_bright);
          cnt1 += 3;

          if (test_mode == 1) {
            strip[i][j] = CRGB(0 * lfo_bright, 0 * lfo_bright, lfo[0] * lfo_bright);
            //strip[i][j] = CRGB(10,0,0);
            if (i + 1 <= j) {
              //strip[i][j] = CRGB(random(100), 10, 10);
              strip[i][j] = CRGB(10, 20, 10);
            }
            if (strand_count == i && ball_count == j) {
              byte lvl = 0;
              strip[i][j] = CRGB(lvl, lvl, lvl);
            }

          }
        }
      }
    }


    if (incoming_universe == 2 || test_mode == 1) {
      //Serial.println("  u1");
      for (int i = start_address; i < number_of_channels; i++) {
        channel_buffer[i - start_address] = byte(packetBuffer[i + art_net_header_size + 1]);
      }

      for (int i = 0; i < number_of_channels; i++) {
        buff2[i + ((incoming_universe - first_universe_number)*number_of_channels)] = channel_buffer[i - start_address];
      }

      int sel = 0;
      int cnt1 = 0;
      for (int i = 7; i < stripCount + 1; i++) {
        for (int j = 0; j < ledsPerStrip; j++) {
          sel = cnt1 + (number_of_channels * 2);
          strip[i][j] = CRGB( buff2[sel] * max_bright, buff2[sel + 1] * max_bright, buff2[sel + 2] * max_bright);
          cnt1 += 3;

          if (test_mode == 1) {
            strip[i][j] = CRGB(0 * lfo_bright, lfo[2] * lfo_bright, 0 * lfo_bright);
            //strip[i][j] = CRGB(10,0,0);
            if (i - 7 + 1 <= j) {
              byte rr = random(max_bright * (255 * i * j));
              byte gg = random(max_bright * (255 * i * j));
              byte bb = random(max_bright * (255 * i * j));

              strip[i][j] = CRGB(50, 10, 20);
            }
            if (strand_count == i && ball_count == j) {
              byte lvl = 0;
              strip[i][j] = CRGB(lvl, lvl, lvl);
            }
          }
        }
      }
    }
    FastLED.show();

  }

  if (cm - prev[1] > 100) {
    prev[1] = cm;
    if (test_mode == 1) {
      ball_count++;
      if ( ball_count > ledsPerStrip) {
        strand_count++;
        ball_count = 0;
      }
      if ( strand_count > stripCount) {
        strand_count = 0;
      }

      Serial.print(ball_count);
      Serial.print(" ");
      Serial.print(strand_count);
      Serial.println(" ");
      Serial.println(" ");
    }

    if (test_mode == 0) {

      if (printer == 1) {
        for (int j = 0; j < 12; j++) {
          Serial.print(buff2[j]);
          Serial.print(" ");
        }

        Serial.print(" - ");
        for (int j = 0; j < number_of_channels; j++) {
          Serial.print(buff2[j + number_of_channels * 2]);
          Serial.print(" ");
        }


        Serial.print(" 6- ");
        for (int j = 13 * 5; j < 13 * 6; j++) {
          Serial.print(buff2[j + number_of_channels * 2]);
          Serial.print(" ");
        }
        Serial.print(" 7- ");
        for (int j = 13 * 7; j < 13 * 8; j++) {
          Serial.print(buff2[j + number_of_channels * 2]);
          Serial.print(" ");
        }
        Serial.println(" ");
      }
    }
  }


  if (cm - prev[2] > 20 && test_mode == 1) {
    prev[2] = cm;
    if (test_mode == 1) {
      for (int i = 0; i < 3; i++) {

        if (latch[i] == 0) {
          lfo[i] += 10 + (i * 3);
        }
        if (latch[i] == 1) {
          lfo[i] -= 10 + (i * 6);
        }
        if (lfo[i] > 254) {
          lfo[i] = (254);
          latch[i] = 1;
        }
        if (lfo[i] < 1) {
          lfo[i] = 0;
          latch[i] = 0;
        }
      }
    }
  }
}
