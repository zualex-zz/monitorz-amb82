#include "WiFiServer.h"
#ifndef WEBSERVERZ_H
#define WEBSERVERZ_H

#include <Arduino.h>
#include <WiFi.h>
// #include "AmebaFatFS.h"
#include "index.h"

#define PART_BOUNDARY "123456789000000000000987654321"

class Webserverz {

  char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
  char* IMG_HEADER = "Content-Type: image/jpeg\r\nContent-Length: %lu\r\n\r\n";

  WiFiServer* webServer;
  WiFiServer* streamServer;

  // AmebaFatFS fs;
  // char* indexFileName = "index.html";    // name of the HTML file saved in SD card

  int channel = 0;

  uint32_t img_addr = 0;
  uint32_t img_len = 0;

  void sendHeader(WiFiClient& client) {
    client.print("HTTP/1.1 200 OK\r\nContent-type: multipart/x-mixed-replace; boundary=");
    client.println(PART_BOUNDARY);
    client.print("Transfer-Encoding: chunked\r\n");
    client.print("\r\n");
  }

  void sendChunk(WiFiClient& client, uint8_t* buf, uint32_t len) {
    uint8_t chunk_buf[64] = { 0 };
    uint8_t chunk_len = snprintf((char*)chunk_buf, 64, "%lX\r\n", len);
    client.write(chunk_buf, chunk_len);
    client.write(buf, len);
    client.print("\r\n");
  }

public:
  Webserverz(int ch = 0) {
    channel = ch;
    webServer = new WiFiServer(80);
    webServer->begin();
    streamServer = new WiFiServer(81);
    streamServer->begin();
  }

  void loop() {
    loopStreamServer();
    loopWebServer();
  }

  void loopWebServer() {
    // char html_filepath[128];

    WiFiClient client = webServer->available();  // listen for incoming clients

    if (client) {                    // if you get a client,
      Serial.println("new webServer client");  // print a message out the serial port
      String currentLine = "";       // make a String to hold incoming data from the client
      while (client.connected()) {   // loop while the client's connected
        if (client.available()) {    // if there's bytes to read from the client,
          char c = client.read();    // read a byte, then
          Serial.write(c);           // print it out the serial monitor
          if (c == '\n') {           // if the byte is a newline character

            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
              // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
              // and a content-type so the client knows what's coming, then a blank line:
              client.println("HTTP/1.1 200 OK");
              client.println("Content-type:text/html");
              client.println();

              // fs.begin();

              // sprintf(html_filepath, "%s%s", fs.getRootPath(), indexFileName);
              // File file = fs.open(html_filepath);

              // if (file) {
              //   while (file.available()) {
              //     client.write(file.read());
              //   }
              //   file.close();
              // }
              // fs.end();

              client.println(INDEX_page);
              Serial.println(INDEX_page);
              // break out of the while loop:
              break;
            } else {  // if you got a newline, then clear currentLine:
              currentLine = "";
            }
          } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
          }
        }
      }
      // close the connection:
      client.stop();
      Serial.println("webServer client disconnected");
    }
  }

  void loopStreamServer() {
    WiFiClient client = streamServer->available();

    if (client) {
      Serial.println("new streamServer client connected");
      String currentLine = "";
      while (client.connected()) {
        // Serial.print("c");
        if (client.available()) {
          char c = client.read();
          Serial.write(c);
          // Serial.println("End resp");
          if (c == '\n') {
            if (currentLine.length() == 0) {
              Serial.println("sTREAMING!");
              sendHeader(client);
              while (client.connected()) {
                Camera.getImage(channel, &img_addr, &img_len);
                Serial.print("C");
                uint8_t chunk_buf[64] = { 0 };
                uint8_t chunk_len = snprintf((char*)chunk_buf, 64, IMG_HEADER, img_len);
                sendChunk(client, chunk_buf, chunk_len);
                sendChunk(client, (uint8_t*)img_addr, img_len);
                sendChunk(client, (uint8_t*)STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
                delay(5);  // Increase this delay for higher resolutions to get a more consistent, but lower frame rate
              }
              Serial.print("disconnected!!!");
              break;
            } else {
              currentLine = "";
            }
          } else if (c != '\r') {
            currentLine += c;
          }
        }
      }
      client.stop();
      Serial.println("streamServer client disconnected");
    } else {
      Serial.println("streamServer waiting for client connection");
      // delay(1000);
    }
  }
};

#endif