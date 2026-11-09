/*
www.makerguides.com
https://www.makerguides.com/playing-audio-with-esp32-and-max98357/
Libraries:
- ESP32 Core 2.0.17
- [arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools) 
  Version: 1.2.2
- [arduino-libhelix](https://github.com/pschatzmann/arduino-libhelix)
  Version: 0.9.2
*/

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "settings.h"

// MAX98357 I2S pins
#define MAX_DIN   3
#define MAX_LRC   1
#define MAX_BCLK  2

// Text to Speech
#define TTS_HOST "translate.google.com"
#define TTS_LANG "en"
#define TTS_VOLUME 0.8

// WiFi credentials
const char* ssid     = WIFI_SSID;
const char* password = WIFI_PASSWORD;

WiFiClientSecure client;
I2SStream i2s;
VolumeStream volume(i2s);
EncodedAudioStream mp3decode(&volume, new MP3DecoderHelix());
StreamCopy copier(mp3decode, client);

String urlEncode(const char* text) {
  String out;
  char buf[4];
  for (const char* c = text; *c; c++) {
    if (isalnum((unsigned char)*c) || *c == '-' || *c == '_' || *c == '.' || *c == '~') {
      out += *c;
    } else {
      snprintf(buf, sizeof(buf), "%%%02X", (unsigned char)*c);
      out += buf;
    }
  }
  return out;
}

// Free, keyless TTS via Google Translate (unofficial, max ~200 characters)
void text2speech(const char* text) {
  client.setInsecure();

  if (!client.connect(TTS_HOST, 443)) {
    Serial.println("Connection failed");
    return;
  }

  client.println("GET /translate_tts?ie=UTF-8&client=tw-ob&tl=" TTS_LANG "&q=sv" + urlEncode(text) + " HTTP/1.0");
  client.println("Host: " TTS_HOST);
  client.println("User-Agent: Mozilla/5.0");
  client.println("Connection: close");
  client.println();

  // ---- Skip HTTP headers ----
  bool first = true;
  bool ok = false;
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (first) { Serial.println("HTTP: " + line); ok = line.indexOf(" 200") > 0; first = false; }
    if (line == "\r" || line.length() == 0) break;
  }
  if (!ok) {
    Serial.println(client.readString());
    client.stop();
  }
}

void setup() {
  Serial.begin(115200);
  delay(4000);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);

  Serial.println("Connecting to WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  WiFi.begin(ssid, password);
  // Many Super Mini clones fail to associate at full TX power
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.printf("status=%d\n", WiFi.status());
    if (++tries % 20 == 0) {
      Serial.println("Retrying; scanning networks:");
      int n = WiFi.scanNetworks();
      for (int i = 0; i < n; i++) Serial.printf("  %s (%d dBm)\n", WiFi.SSID(i).c_str(), WiFi.RSSI(i));
      WiFi.begin(ssid, password);
    }
  }
  Serial.println("\nWiFi connected: " + WiFi.localIP().toString());

  auto config = i2s.defaultConfig(TX_MODE);
  config.pin_bck  = MAX_BCLK;
  config.pin_ws   = MAX_LRC;
  config.pin_data = MAX_DIN;
  i2s.begin(config);

  mp3decode.begin();
  volume.begin(config);
  volume.setVolume(TTS_VOLUME);

  text2speech("Hello, this a test for text to speech.");
}

void loop() {
  copier.copy();
}