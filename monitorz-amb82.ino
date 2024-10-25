#include "WiFi.h"
#include "VideoStream.h"
#include "StreamIO.h"
#include "RTSP.h"
#include "MotionDetection.h"
#include "VideoStreamOverlay.h"
#include "telegramz.h"
#include "Base64.h"

#define CHANNEL 0    // High resolution video channel for streaming
#define CHANNELJPEG 1    // jpeg snapshot
#define CHANNELMD 3  // RGB format video for motion detection only available on channel 3

VideoSetting config(VIDEO_FHD, 30, VIDEO_H264, 0);   // High resolution video for streaming
VideoSetting configJPG(VIDEO_FHD, CAM_FPS, VIDEO_JPEG, 1);
VideoSetting configMD(VIDEO_VGA, 10, VIDEO_RGB, 0);  // Low resolution RGB video for motion detection
RTSP rtsp;
StreamIO videoStreamer(1, 1);
StreamIO videoStreamerMD(1, 1);
MotionDetection MD;
Telegramz *telegramz;

uint32_t TRIGGERtimer = 0;      // used for limiting camera motion trigger rate
uint16_t TriggerLimitTime = 2;  // min time between motion detection trigger events (seconds)

int encodedLen;
char *encodedData;
uint32_t img_addr = 0;
uint32_t img_len = 0;

char ssid[] = "ssid";      // your network SSID (name)
char pass[] = "pass";  // your network password
int status = WL_IDLE_STATUS;

void mdPostProcess(std::vector<MotionDetectionResult> md_results) {
  // Motion detection results is expressed as an array
  // With 0 or 1 in each element indicating presence of motion
  // Iterate through all elements to check for motion
  // and calculate rectangles containing motion

  OSD.createBitmap(CHANNEL);
  if (MD.getResultCount() > 0) {
    Serial.print("Motion: ");
    Serial.println(MD.getResultCount());
    for (uint16_t i = 0; i < MD.getResultCount(); i++) {
      MotionDetectionResult result = md_results[i];
      int xmin = (int)(result.xMin() * config.width());
      int xmax = (int)(result.xMax() * config.width());
      int ymin = (int)(result.yMin() * config.height());
      int ymax = (int)(result.yMax() * config.height());
      // printf("%d:\t%d %d %d %d\n\r", i, xmin, xmax, ymin, ymax);
      OSD.drawRect(CHANNEL, xmin, ymin, xmax, ymax, 3, COLOR_GREEN);
      Serial.print(i);
    }
    Serial.println();
    Camera.getImage(CHANNELJPEG, &img_addr, &img_len);
    // encodejpg();
    if ((unsigned long)(millis() - TRIGGERtimer) >= (TriggerLimitTime * 1000)) {  // limit time between triggers
      TRIGGERtimer = millis();                                                    // update last trigger time
      Serial.println("Motion detected!");
      // telegramz->send("Motion");
      telegramz->sendPhotoAs((uint8_t *)img_addr, img_len);
    }
  }
  OSD.update(CHANNEL);
}

void encodejpg() {
  // Encode the file data as Base64
  encodedLen = base64_enc_len(img_len);
  encodedData = (char *)malloc(encodedLen);
  base64_encode(encodedData, (char *)img_addr, img_len);

  Serial.println("img_len");
  Serial.println(img_len);
  Serial.println("encodedLen");
  Serial.println(encodedLen);
  Serial.println("encodedData");
  Serial.println(encodedData);
}

void setup() {
  Serial.begin(115200);

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass);

    // wait 2 seconds for connection:
    delay(2000);
  }

  // Configure camera video channels for required resolutions and format outputs
  // Adjust the bitrate based on your WiFi network quality
  config.setBitrate(2 * 1024 * 1024);  // Recommend to use 2Mbps for RTSP streaming to prevent network congestion
  Camera.configVideoChannel(CHANNEL, config);
  Camera.configVideoChannel(CHANNELJPEG, configJPG);
  Camera.configVideoChannel(CHANNELMD, configMD);
  Camera.videoInit();

  // Configure RTSP for high resolution video stream information
  rtsp.configVideo(config);
  rtsp.begin();

  // Configure motion detection for low resolution RGB video stream
  MD.configVideo(configMD);
  MD.setResultCallback(mdPostProcess);
  MD.begin();

  // Configure StreamIO object to stream data from high res video channel to RTSP
  videoStreamer.registerInput(Camera.getStream(CHANNEL));
  videoStreamer.registerOutput(rtsp);
  if (videoStreamer.begin() != 0) {
    Serial.println("StreamIO link start failed");
  }

  // Start data stream from high resolution video channel
  Camera.channelBegin(CHANNEL);
  Serial.println("Video RTSP Streaming Start");

  // Configure StreamIO object to stream data from low res video channel to motion detection
  videoStreamerMD.registerInput(Camera.getStream(CHANNELMD));
  videoStreamerMD.setStackSize();
  videoStreamerMD.registerOutput(MD);
  if (videoStreamerMD.begin() != 0) {
    Serial.println("StreamIO link start failed");
  }

  // Start data stream from low resolution video channel
  Camera.channelBegin(CHANNELMD);

  // Configure OSD for drawing on high resolution video stream
  OSD.configVideo(CHANNEL, config);
  OSD.begin();

  Camera.channelBegin(CHANNELJPEG);

  telegramz = new Telegramz();
  telegramz->addAction("/takePicture", [](Telegramz *t) {
    Serial.println("take picture");
    // camz->setFramesizePhoto();
    // camera_fb_t *fb = camz->takePhoto();
    // t->sendPhoto(fb);
    // camz->reuseBuffer(fb);
    // camz->setFramesizeMotion();
  });
}

void loop() {
  // Do nothing
}
