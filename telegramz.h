#ifndef TELEGRAMZ_H
#define TELEGRAMZ_H

#define TELEGRAM_BOTNAME "BOTNAME"
#define TELEGRAM_BOTUSERNAME "BOTUSERNAME"
#define TELEGRAM_BOTTOKEN "BOTTOKEN"
#define TELEGRAM_CHATID 1234567

#define telegramServer "api.telegram.org"

#define BLOCK_SIZE 1436  // 2872   // 2 * TCP_MSS

#include <Arduino.h>
#include <WiFiClient.h>
#include <vector>

class Telegramz {
  const char *botName = TELEGRAM_BOTNAME;
  const char *botUsername = TELEGRAM_BOTUSERNAME;
  const char *botToken = TELEGRAM_BOTTOKEN;
  const int64_t chatId = TELEGRAM_CHATID;
  const unsigned long BOT_MTBS = 1000;  // mean time between scan messages

  const char *httpGetSendMessage = "GET /bot%s/sendMessage?chat_id=%lld&text=%s  HTTP/1.1";
  const char *httpPostSendPhoto = "POST /bot%s/sendPhoto  HTTP/1.1";

  unsigned long bot_lasttime;  // last time messages' scan has been done
  bool Start = false;

  WiFiSSLClient client;

  typedef void (*TelegramCallBack)(Telegramz *);

  typedef struct Action {
    String text;
    TelegramCallBack execute;
  } Action;

  std::vector<Action> actions = {};

public:
  Telegramz() {
  }

  void addAction(String text, TelegramCallBack toExecute) {
    actions.push_back({ text, toExecute });
  }

  void send(const String &message) {
    char httpMethod[300];
    sprintf(httpMethod, httpGetSendMessage, botToken, chatId, message.c_str());

    if (client.connect(telegramServer, 443)) {
      Serial.println("connected");
      client.println(httpMethod);
      client.println("Host: " + String(telegramServer));
      client.println("Connection: close");
      client.println();

      while (client.available()) {
        char c = client.read();
        Serial.write(c);
      }

      Serial.println("sent");
    } else {
      Serial.println("connection error");
    }
  }

  void sendPhoto(int encodedLen, char *encodedData) {
    // bot->sendPhoto(chatId, fb->buf, fb->len);
    char httpMethod[300];
    sprintf(httpMethod, httpPostSendPhoto, botToken);
    Serial.println(httpMethod);

    if (client.connect(telegramServer, 443)) {
      Serial.println("connected");
      // client.println("POST " + url + " HTTP/1.1");
      client.println(httpMethod);
      client.println("Host: " + String(telegramServer));
      client.println("Content-Type: multipart/form-data; boundary=boundary");
      client.println("Connection: keep-alive");

      // Calculate content length
      size_t contentLength = encodedLen + strlen("--boundary--\r\n");
      client.print("Content-Length: ");
      client.println(contentLength);

      char int64_buf[22] = { 0 };
      snprintf(int64_buf, sizeof(int64_buf), "%lld", chatId);

      client.println("--boundary");
      client.println("Content-Disposition: form-data; name=\"chat_id\"");
      client.println();
      client.println(int64_buf);

      // Send multipart/form-data body
      client.println("--boundary");
      client.println("Content-Disposition: form-data; name=\"photo\"; filename=\"image.jpg\"");
      client.println("Content-Type: image/jpg");
      client.println();
      client.print(encodedData);
      client.println();
      client.println("--boundary--");

      Serial.println("sent");

      while (client.available()) {
        char c = client.read();
        Serial.write(c);
      }

    } else {
      Serial.println("connection error");
    }
  }

  void setformData(int64_t chat_id, const char *cmd, const char *type,
                   const char *propName, size_t size, String &formData,
                   String &request, const char *filename, const char *caption) {

#define BOUNDARY "----WebKitFormBoundary7MA4YWxkTrZu0gW"
#define END_BOUNDARY "\r\n--" BOUNDARY "--\r\n"

    char int64_buf[22] = { 0 };
    snprintf(int64_buf, sizeof(int64_buf), "%lld", chat_id);

    formData = "--" BOUNDARY "\r\nContent-disposition: form-data; name=\"chat_id\"\r\n\r\n";
    formData += int64_buf;

    if (caption != nullptr) {
      formData += "\r\n--" BOUNDARY "\r\nContent-disposition: form-data; name=\"caption\"\r\n\r\n";
      formData += caption;
    }

    formData += "\r\n--" BOUNDARY "\r\nContent-disposition: form-data; name=\"";
    formData += propName;
    formData += "\"; filename=\"";
    formData += filename != nullptr ? filename : "image.jpg";
    formData += "\"\r\nContent-Type: ";
    formData += type;
    formData += "\"\r\n\r\n";

    request = "POST /bot";
    request += botToken;
    request += "/";
    request += cmd;
    request += " HTTP/1.0"
               "\r\nConnection: keep-alive"
               "\r\nHost: " telegramServer
               "\r\nContent-Length: ";
    request += (size + formData.length() + strlen(END_BOUNDARY));
    request += "\r\nContent-Type: multipart/form-data; boundary=" BOUNDARY "\r\n";

    // Serial.println("request");
    // Serial.println(request);
    // Serial.println(formData);
  }

  void sendPhotoAs(uint8_t *data, int size) {
    if (client.connect(telegramServer, 443)) {
      String formData;
      formData.reserve(512);
      String request;
      request.reserve(256);
      setformData(chatId, "sendPhoto", "image/jpeg", "photo", size, formData, request, nullptr, "Motion!");

#if DEBUG_ENABLE
      uint32_t t1 = millis();
#endif
      // Send POST request to host
      client.println(request);
      // Body of request
      client.print(formData);

      // Serial.println(client.write((const uint8_t *)data, size));
      uint16_t pos = 0;
      int n_block = trunc(size / BLOCK_SIZE);
      int lastBytes = size - (n_block * BLOCK_SIZE);

      // Serial.println(size);
      // Serial.println(n_block);
      // Serial.println(lastBytes);

      for (pos = 0; pos < n_block; pos++) {
        client.write((const uint8_t *)data + pos * BLOCK_SIZE, BLOCK_SIZE);
        yield();
      }
      client.write((const uint8_t *)data + pos * BLOCK_SIZE, lastBytes);

      // Close the request form-data
      client.println(END_BOUNDARY);

#if DEBUG_ENABLE
      Serial.printf("Raw upload time: %lums\n", millis() - t1);
      t1 = millis();
#endif
      // Serial.println("\nSended!");
      while (client.available()) {
        char c = client.read();
        Serial.write(c);
      }

    } else
      Serial.println("\nError: client not connected");
  }

  void loop() {
    // if there is an incoming message...
  }
};

#endif