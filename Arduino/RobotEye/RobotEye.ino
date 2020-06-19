#include <ArduinoWebsockets.h>
#include "esp_http_server.h"
#include "esp_timer.h"
#include "esp_camera.h"
#include "camera_index.h" // this is the index.html page gzipped then converted to hex
#include "Arduino.h"

#include <Wire.h> // for i2c communication

const char* ssid = "<ssid>";
const char* password = "<password>";

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT
//#define CAMERA_MODEL_ESP_EYE
//#define CAMERA_MODEL_M5STACK_PSRAM
//#define CAMERA_MODEL_M5STACK_WIDE
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

using namespace websockets;
WebsocketsServer socket_server;

camera_fb_t * fb = NULL;

long current_millis;
long last_detected_millis = 0;

void app_httpserver_init();

httpd_handle_t camera_httpd = NULL;

// pins to communicate with arduino
#define PIN_SDA 2
#define PIN_SCL 12
#define I2C_BUS 0x09 // arbitrary unused address in I2C

//#define PIN_FORWARD 2
//#define PIN_REVERSE 12
//#define PIN_LEFT 13
//#define PIN_RIGHT 15

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  // start the i2c bus as master
  Wire.begin(PIN_SDA, PIN_SCL); // use specified pins for i2c because sda and scl pins are not exposed in esp32-cam

// code below is from CameraServer example code
  camera_config_t config;
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
  //init with high specs to pre-allocate larger buffers
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  app_httpserver_init();
  socket_server.listen(82);

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_set_type(req, "text/html");
  httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
  return httpd_resp_send(req, (const char *)index_ov2640_html_gz, index_ov2640_html_gz_len);
}

httpd_uri_t index_uri = {
  .uri       = "/",
  .method    = HTTP_GET,
  .handler   = index_handler,
  .user_ctx  = NULL
};

void app_httpserver_init ()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  if (httpd_start(&camera_httpd, &config) == ESP_OK)
    Serial.println("httpd_start");
  {
    httpd_register_uri_handler(camera_httpd, &index_uri);
  }
}

// sending commands to the arduino using I2C wires
void handle_message(WebsocketsClient &client, WebsocketsMessage msg)
{
  Wire.beginTransmission(I2C_BUS);
  if (msg.data() == "stop") {
    Wire.write(0);
    client.send("STOPPING");
  }
  if (msg.data() == "forward") {
    Wire.write(1);
    client.send("MOVING");
  }
  if (msg.data() == "reverse") {
    Wire.write(2);
    client.send("REVERSING");
  }
  if (msg.data() == "left") {
    Wire.write(3);
    client.send("TURNING LEFT");
  }
  if (msg.data() == "right") {
    Wire.write(4);
    client.send("TURNING RIGHT");
  }
  if (msg.data() == "fire") {
    Wire.write(5);
    client.send("FIRING");
  }
  if (msg.data() == "laser") {
    Wire.write(6);
    client.send("TOGGLING LASER");
  }
  if (msg.data() == "nleft") {
    Wire.write(7);
    client.send("NUDGING LEFT");
  }
  if (msg.data() == "nright") {
    Wire.write(8);
    client.send("NUDGING RIGHT");
  }
  Wire.endTransmission();
}

void loop() {
  auto client = socket_server.accept();
  client.onMessage(handle_message);

  client.send("STREAMING");

  while (client.available()) {
    client.poll();

    fb = esp_camera_fb_get();

    client.sendBinary((const char *)fb->buf, fb->len);

    esp_camera_fb_return(fb);
    fb = NULL;
  }
}
