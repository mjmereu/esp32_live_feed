// ESP32-S3 USB live view: streams camera frames over the USB cable (no WiFi) to viewer.py,
// so you can twist the lens and watch the image change.
//
// Arduino IDE: Board "ESP32S3 Dev Module", PSRAM "OPI PSRAM", USB CDC On Boot "Disabled".
// Plug into the port labelled TTL (not OTG). Close the Serial Monitor before running viewer.py.

#include "esp_camera.h"

#define BAUD 921600   // raise to 2000000 for a smoother picture if your USB-serial chip allows it
                      // (must match --baud in viewer.py)

// Pins for the ESP32-S3-EYE style camera wiring (your OV3660 board).
#define PWDN_GPIO_NUM  -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  15
#define SIOD_GPIO_NUM   4
#define SIOC_GPIO_NUM   5
#define Y9_GPIO_NUM    16
#define Y8_GPIO_NUM    17
#define Y7_GPIO_NUM    18
#define Y6_GPIO_NUM    12
#define Y5_GPIO_NUM    10
#define Y4_GPIO_NUM     8
#define Y3_GPIO_NUM     9
#define Y2_GPIO_NUM    11
#define VSYNC_GPIO_NUM  6
#define HREF_GPIO_NUM   7
#define PCLK_GPIO_NUM  13

// Frame header: 4 magic bytes, then the JPEG length as a 4-byte little-endian number, then the JPEG.
static const uint8_t MAGIC[4] = {0xA5, 0x5A, 0xC3, 0x3C};

static void setSize(framesize_t size) {
  sensor_t* s = esp_camera_sensor_get();
  if (s) s->set_framesize(s, size);
}

void setup() {
  Serial.begin(BAUD);

  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QXGA;      // allocate for the largest size; actual size set below
  config.jpeg_quality = 12;                // 0-63, lower = better/bigger
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (esp_camera_init(&config) != ESP_OK) {
    while (true) {   // no camera: say so in plain text forever (visible in the Serial Monitor)
      Serial.println("Camera init failed. Check Tools > PSRAM = OPI PSRAM and the cable.");
      delay(1000);
    }
  }
  setSize(FRAMESIZE_VGA);                  // start small and fast
}

void loop() {
  // One-letter commands from viewer.py change the picture size.
  while (Serial.available()) {
    switch (Serial.read()) {
      case '1': setSize(FRAMESIZE_QVGA); break;   // 320x240
      case '2': setSize(FRAMESIZE_VGA);  break;   // 640x480
      case '3': setSize(FRAMESIZE_SVGA); break;   // 800x600
      case '4': setSize(FRAMESIZE_XGA);  break;   // 1024x768
      case '5': setSize(FRAMESIZE_SXGA); break;   // 1280x1024
      case '6': setSize(FRAMESIZE_UXGA); break;   // 1600x1200
      case '7': setSize(FRAMESIZE_QXGA); break;   // 2048x1536
    }
  }

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) return;
  uint32_t len = fb->len;
  Serial.write(MAGIC, 4);
  Serial.write((const uint8_t*)&len, 4);
  Serial.write(fb->buf, len);
  esp_camera_fb_return(fb);
}
