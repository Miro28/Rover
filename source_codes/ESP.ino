// ===== esp32.ino =====
// Camera stream + live control webpage
// Non-blocking stream so serial reading never starves

#include "esp_camera.h"
#include <WiFi.h>

const char* ssid     = "alo";
const char* password = "11111111";

// AI Thinker pin map
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

WiFiServer controlServer(80);
WiFiServer streamServer(81);

// persistent stream client - served one frame per loop
WiFiClient streamClient;

// latest sensor values from Mega
int   sT = -1, sH = -1, sP = 0, sR = 0;
long  sD = -1;
String serialBuf = "";

void startCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
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
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
}

void setup() {
  Serial.begin(9600);   // link to Mega Serial1

  startCamera();

  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.println(ssid);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.print("WiFi FAILED. Status code: ");
    Serial.println(WiFi.status());
  }

  controlServer.begin();
  streamServer.begin();
}

void loop() {
  readSerialData();     // always serviced now
  handleControl();
  handleStreamClient(); // sends ONE frame per pass, never blocks
}

// ---- read "T:.. H:.. D:.. P:.. R:.." lines from Mega ----
void readSerialData() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      parseLine(serialBuf);
      serialBuf = "";
    } else if (c != '\r') {
      serialBuf += c;
    }
  }
}

void parseLine(String line) {
  int ti = line.indexOf("T:");
  int hi = line.indexOf("H:");
  int di = line.indexOf("D:");
  int pi = line.indexOf("P:");
  int ri = line.indexOf("R:");
  if (ti<0||hi<0||di<0||pi<0||ri<0) return;

  sT = line.substring(ti+2, hi).toInt();
  sH = line.substring(hi+2, di).toInt();
  sD = line.substring(di+2, pi).toInt();
  sP = line.substring(pi+2, ri).toInt();
  sR = line.substring(ri+2).toInt();
}

void handleControl() {
  WiFiClient client = controlServer.available();
  if (!client) return;

  String req = client.readStringUntil('\r');
  while (client.available()) client.read();

  if (req.indexOf("GET /F") >= 0) { Serial.print('F'); sendOK(client); return; }
  if (req.indexOf("GET /B") >= 0) { Serial.print('B'); sendOK(client); return; }
  if (req.indexOf("GET /L") >= 0) { Serial.print('L'); sendOK(client); return; }
  if (req.indexOf("GET /R") >= 0) { Serial.print('R'); sendOK(client); return; }
  if (req.indexOf("GET /S") >= 0) { Serial.print('S'); sendOK(client); return; }

  if (req.indexOf("GET /data") >= 0) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.print("{\"t\":"); client.print(sT);
    client.print(",\"h\":"); client.print(sH);
    client.print(",\"d\":"); client.print(sD);
    client.print(",\"p\":"); client.print(sP);
    client.print(",\"r\":"); client.print(sR);
    client.println("}");
    client.stop();
    return;
  }

  sendPage(client);
  client.stop();
}

void sendOK(WiFiClient &client) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Connection: close");
  client.println();
  client.print("ok");
  client.stop();
}

void sendPage(WiFiClient &client) {
  String ip = WiFi.localIP().toString();
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  client.println();

  client.println("<!DOCTYPE html><html><head>");
  client.println("<meta name='viewport' content='width=device-width,initial-scale=1'>");
  client.println("<title>Mars Rover</title><style>");
  client.println("body{font-family:sans-serif;text-align:center;background:#1a1a1a;color:#eee;}");
  client.println("button{width:90px;height:90px;font-size:26px;margin:5px;border-radius:10px;");
  client.println("border:none;background:#c1440e;color:#fff;}");
  client.println("button:active{background:#e0651e;}");
  client.println("#danger{font-size:22px;font-weight:bold;color:#ff3030;height:26px;}");
  client.println(".val{font-size:20px;margin:6px;}");
  client.println("</style></head><body>");

  client.println("<h2>Mars Rover Control</h2>");

  client.print("<img src='http://");
  client.print(ip);
  client.println(":81/' width='400'><br>");

  client.println("<div class='val'>Temperature: <span id='t'>--</span> &deg;C</div>");
  client.println("<div class='val'>Humidity: <span id='h'>--</span> %</div>");
  client.println("<div class='val'>Distance: <span id='d'>--</span> cm</div>");
  client.println("<div id='danger'></div>");

  client.println("<svg width='220' height='140'>");
  client.println("<g id='rover' transform='rotate(0 110 80)'>");
  client.println("<rect x='60' y='60' width='100' height='30' rx='6' fill='#c1440e'/>");
  client.println("<circle cx='80' cy='95' r='12' fill='#888'/>");
  client.println("<circle cx='140' cy='95' r='12' fill='#888'/>");
  client.println("<rect x='95' y='40' width='12' height='22' fill='#888'/>");
  client.println("<line x1='30' y1='80' x2='190' y2='80' stroke='#444' stroke-width='1'/>");
  client.println("</g></svg>");
  client.println("<div class='val'>Tilt: <span id='tilt'>0</span>&deg;</div>");

  client.println("<div>");
  client.println("<button onclick=\"cmd('F')\">F</button><br>");
  client.println("<button onclick=\"cmd('L')\">L</button>");
  client.println("<button onclick=\"cmd('S')\">S</button>");
  client.println("<button onclick=\"cmd('R')\">R</button><br>");
  client.println("<button onclick=\"cmd('B')\">B</button>");
  client.println("</div>");

  client.println("<script>");
  client.println("function cmd(c){fetch('/'+c);}");
  client.println("function upd(){fetch('/data').then(r=>r.json()).then(d=>{");
  client.println("document.getElementById('t').innerText=d.t;");
  client.println("document.getElementById('h').innerText=d.h;");
  client.println("document.getElementById('d').innerText=d.d;");
  client.println("var dg=document.getElementById('danger');");
  client.println("if(d.d>=0 && d.d<10){dg.innerText='\\u26A0 DANGER - OBSTACLE';}else{dg.innerText='';}");
  client.println("document.getElementById('tilt').innerText=d.p;");
  client.println("document.getElementById('rover').setAttribute('transform','rotate('+(-d.p)+' 110 80)');");
  client.println("});}");
  client.println("setInterval(upd,1000);");
  client.println("</script>");

  client.println("</body></html>");
}

// ---- non-blocking stream: one frame per loop pass ----
void handleStreamClient() {
  // accept a new stream client if none active
  if (!streamClient || !streamClient.connected()) {
    WiFiClient newClient = streamServer.available();
    if (newClient) {
      streamClient = newClient;
      streamClient.println("HTTP/1.1 200 OK");
      streamClient.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
      streamClient.println();
    }
  }

  // send exactly one frame, then return to loop
  if (streamClient && streamClient.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (fb) {
      streamClient.println("--frame");
      streamClient.println("Content-Type: image/jpeg");
      streamClient.print("Content-Length: ");
      streamClient.println(fb->len);
      streamClient.println();
      streamClient.write(fb->buf, fb->len);
      streamClient.println();
      esp_camera_fb_return(fb);
    }
  }
}