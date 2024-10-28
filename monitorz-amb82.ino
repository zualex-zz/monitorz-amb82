#include "WiFi.h"
#include "StreamIO.h"
#include "VideoStream.h"
#include "AudioStream.h"
#include "AudioEncoder.h"
#include "MP4Recording.h"
#include "RTSP.h"
#include "MotionDetection.h"
#include "VideoStreamOverlay.h"
#include "telegramz.h"
#include "webserverz.h"

// 37,5 x 60 mm board

#define CHANNEL 0      // High resolution video channel for streaming
#define CHANNELJPEG 1  // jpeg snapshot
#define CHANNELMD 3    // RGB format video for motion detection only available on channel 3


#define GREEN_LED 4

AudioSetting configA(0);
VideoSetting config(VIDEO_FHD, 30, VIDEO_H264, 0);  // High resolution video for streaming
VideoSetting configJPG(VIDEO_FHD, CAM_FPS, VIDEO_JPEG, 1);
VideoSetting configMD(VIDEO_VGA, 10, VIDEO_RGB, 0);  // Low resolution RGB video for motion detection

Audio audio;
AAC aac;
MP4Recording mp4;
RTSP rtsp;
MotionDetection MD;
// StreamIO videoStreamer(1, 1);
StreamIO videoStreamerMD(1, 1);
StreamIO audioStreamer(1, 1);      // 1 Input Audio -> 1 Output AAC
StreamIO avMixStreamer(2, 2);      // 2 Input Video + Audio -> 2 Output MP4 + RTSP

Telegramz *telegramz;
Webserverz *webserverz;

uint32_t TRIGGERtimer = 0;      // used for limiting camera motion trigger rate
uint16_t TriggerLimitTime = 2;  // min time between motion detection trigger events (seconds)

uint32_t img_addr = 0;
uint32_t img_len = 0;

bool record = true;
int recordingCount = 0;

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
    digitalWrite(GREEN_LED, HIGH);
    // Serial.print("Motion: ");
    // Serial.println(MD.getResultCount());
    for (uint16_t i = 0; i < MD.getResultCount(); i++) {
      MotionDetectionResult result = md_results[i];
      int xmin = (int)(result.xMin() * config.width());
      int xmax = (int)(result.xMax() * config.width());
      int ymin = (int)(result.yMin() * config.height());
      int ymax = (int)(result.yMax() * config.height());
      // printf("%d:\t%d %d %d %d\n\r", i, xmin, xmax, ymin, ymax);
      OSD.drawRect(CHANNEL, xmin, ymin, xmax, ymax, 3, COLOR_GREEN);
    }

    if (record) {
      if (!mp4.getRecordingState()) {    // if not MP4 not in recording mode
        recordingCount++;
        mp4.setRecordingFileName("MotionDetection" + String(recordingCount));
        mp4.begin();
        // tone(BUZZER_PIN, 1000, 500);
      }
    } else {
      if ((unsigned long)(millis() - TRIGGERtimer) >= (TriggerLimitTime * 1000)) {  // limit time between triggers
        TRIGGERtimer = millis();                                                    // update last trigger time
        Serial.println("Motion triggered!");
        // telegramz->send("Motion");
        Camera.getImage(CHANNELJPEG, &img_addr, &img_len);
        telegramz->sendPhotoAs((uint8_t *)img_addr, img_len);
      }
    }
  }
  OSD.update(CHANNEL);
}

void setup() {
  Serial.begin(115200);
    // GPIO Initialization
    pinMode(GREEN_LED, OUTPUT);

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

    // Configure audio peripheral for audio data output
    audio.configAudio(configA);
    audio.begin();
    // Configure AAC audio encoder
    aac.configAudio(configA);
    aac.begin();
  // Configure RTSP for high resolution video stream information
  rtsp.configVideo(config);
  rtsp.configAudio(configA, CODEC_AAC);
  rtsp.begin();

  // Configure motion detection for low resolution RGB video stream
  MD.configVideo(configMD);
  MD.setResultCallback(mdPostProcess);
  MD.begin();

    // Configure MP4 with identical video format information
    // Configure MP4 recording settings
    mp4.configVideo(config);
    mp4.configAudio(configA, CODEC_AAC);
    mp4.setRecordingDuration(30);
    mp4.setRecordingFileCount(1);
    // Configure StreamIO object to stream data from audio channel to AAC encoder
    audioStreamer.registerInput(audio);
    audioStreamer.registerOutput(aac);
    if (audioStreamer.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }

    // Configure StreamIO object to stream data from video channel and AAC encoder to MP4 recording
    avMixStreamer.registerInput1(Camera.getStream(CHANNEL));
    avMixStreamer.registerInput2(aac);
    avMixStreamer.registerOutput1(rtsp);
    avMixStreamer.registerOutput2(mp4);
    if (avMixStreamer.begin() != 0) {
        Serial.println("StreamIO link start failed");
    }

    // Start data stream from video channel
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
  });

  webserverz = new Webserverz(CHANNELJPEG);
}

void loop() {
  webserverz->loop();
}
