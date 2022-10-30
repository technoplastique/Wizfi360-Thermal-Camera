/*
  WizFi360 example: WebServerLed

  A simple web server that lets you turn on and of an LED via a web page.
  This sketch will print the IP address of your WizFi360 module (once connected)
  to the Serial monitor. From there, you can open that address in a web browser
  to turn on and off the LED on pin 13.
*/

#include "WizFi360.h"
#include <stdio.h>
#include "pico/stdlib.h"
#include "arducampico.h"
uint8_t header[2] = {0x55, 0xAA};
uint8_t image[96 * 96] = {0}; //96*96
struct arducam_config config;

// setup according to the device you use
#define WIZFI360_EVB_PICO

// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
#if defined(ARDUINO_MEGA_2560)
SoftwareSerial Serial1(6, 7); // RX, TX
#elif defined(WIZFI360_EVB_PICO)
SoftwareSerial Serial2(6, 7); // RX, TX
#endif
#endif

/* Baudrate */
#define SERIAL_BAUDRATE   115200
#if defined(ARDUINO_MEGA_2560)
#define SERIAL1_BAUDRATE  115200
#elif defined(WIZFI360_EVB_PICO)
#define SERIAL2_BAUDRATE  115200
#endif

/* Wi-Fi info */
char ssid[] = "network";       // your network SSID (name), like iPhone
char pass[] = "password1234567890";   // your network password

int status = WL_IDLE_STATUS;  // the Wifi radio's status

int ledStatus = LOW;

WiFiServer server(80);

// use a ring buffer to increase speed and reduce memory allocation
RingBuffer buf(8);

void setup() {
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // initialize serial for debugging
  Serial.begin(SERIAL_BAUDRATE);
  // initialize serial for WizFi360 module
#if defined(ARDUINO_MEGA_2560)
  Serial1.begin(SERIAL1_BAUDRATE);
#elif defined(WIZFI360_EVB_PICO)
  Serial2.begin(SERIAL2_BAUDRATE);
#endif
  // initialize WizFi360 module
#if defined(ARDUINO_MEGA_2560)
  WiFi.init(&Serial1);
#elif defined(WIZFI360_EVB_PICO)
  WiFi.init(&Serial2);
#endif

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");
  printWifiStatus();

  // start the web server on port 80
  server.begin();

  //camera settings
  config.sccb = i2c0;
  config.sccb_mode = I2C_MODE_16_8;
  config.sensor_address = 0x24;
  config.pin_sioc = PIN_CAM_SIOC;
  config.pin_siod = PIN_CAM_SIOD;
  config.pin_resetb = PIN_CAM_RESETB;
  config.pin_xclk = PIN_CAM_XCLK;
  config.pin_vsync = PIN_CAM_VSYNC;
  config.pin_y2_pio_base = PIN_CAM_Y2_PIO_BASE;
  config.pio = pio0;
  config.pio_sm = 0; //this one, 0
  config.dma_channel = 0;
  arducam_init(&config);
}

void loop() {

  //This part makes the camrea work
  //Serial.println("start image");
  arducam_capture_frame(&config, image); //this captures one frame
  Serial.write(header, 2);
  delay(5);
  int img = Serial.write(image, 96 * 96);


  //This part makes the website
  WiFiClient client  = server.available();  // listen for incoming clients

  if (client) {                               // if you get a client,
    Serial.println("New client");             // print a message out the serial port
    buf.init();                               // initialize the circular buffer
    while (client.connected()) {              // loop while the client's connected
      if (client.available()) {               // if there's bytes to read from the client,
        char c = client.read();               // read a byte, then
        buf.push(c);                          // push it to the ring buffer

        // printing the stream to the serial monitor will slow down
        // the receiving of data from the WizFi360 filling the serial buffer
        //Serial.write(c);

        // you got two newline characters in a row
        // that's the end of the HTTP request, so send a response
        if (buf.endsWith("\r\n\r\n")) {
          sendHttpResponse(client);
          break;
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (buf.endsWith("GET /H")) {
          Serial.println("Turn led ON");
          ledStatus = HIGH;
          digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
        }
        else if (buf.endsWith("GET /L")) {
          Serial.println("Turn led OFF");
          ledStatus = LOW;
          digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
        }

      }

    }

    // close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
}

void sendHttpResponse(WiFiClient client) {
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();

  // the content of the HTTP response follows the header:
  client.println("<head><script src=\"https://cdn.jsdelivr.net/npm/p5@1.5.0/lib/p5.js\"></script><script language=\"javascript\" type=\"text/javascript\" src=\"https://cdn.jsdelivr.net/gh/ongzzzzzz/p5.web-serial/lib/p5.web-serial.js\"></script></head><body>");
  client.print("Your Image: ");
  client.println("<br>");
  client.println("<br>");

  client.println("<script>let myPort; final int cameraWidth = 96; final int cameraHeight = 96; final int cameraBytesPerPixel = 1; final int bytesPerFrame = cameraWidth * cameraHeight * cameraBytesPerPixel;PImage myImage;byte[] frameBuffer = new byte[bytesPerFrame];byte[] header = new byte[3];byte[] score = new byte[2];");
  client.println("function setup(){createCanvas(300, 300);myPort = createSerial();myPort.open(115200);myPort.buffer(bytesPerFrame);myImage = createImage(cameraWidth, cameraHeight, GRAY);fill(255, 0, 0);}");
  client.println("function draw(){rect(7,7,20,20);image(myImage, 0, 0);}int state = 0;int read = 0;int result = 0;int startbyte;");
  client.println("function serialEvent(Serial myPort) {if (read == 0) {startbyte = myPort.read();if (startbyte == 0x55) {state = 1;}if (startbyte == 0xAA && state == 1) {read = 1;}if (startbyte == 0xBB && state == 1) {result = 1; }}");
  client.println("if (result == 1) {myPort.readBytes(score);result = 0;}if (read ==1) {myPort.readBytes(frameBuffer);ByteBuffer bb = ByteBuffer.wrap(frameBuffer);bb.order(ByteOrder.BIG_ENDIAN);int i = 0;");
  client.println("while (bb.hasRemaining()) {short p = bb.getShort();int p1 = (p>>8)&0xFF;int p2 = p&0xFF;int r = p1;//((p >> 11) & 0x1f) << 3;int g = p1;//((p >> 5) & 0x3f) << 2;int b = p1;//((p >> 0) & 0x1f) << 3;");
  client.println("myImage .pixels[i++] = color(r, g, b);r = p2;g = p2;b = p2;myImage .pixels[i++] = color(r, g, b);}read = 0;}myImage .updatePixels();}");
  client.println("</script></body>");

  // The HTTP response ends with another blank line:
  client.println();
}

void printWifiStatus() {
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in the browser
  Serial.println();
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
  Serial.println();
}
