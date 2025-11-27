#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <DHT11.h>

const char* ssid="DESKTOP-Q78F81O";
const char* password="abc12345";

#define LED_PIN 14  // D5 pin on NodeMCU (GPIO14)
#define DHT_PIN 2   // D4 pin on NodeMCU (GPIO2)
#define SDA_PIN 4   // D2 pin on NodeMCU (GPIO4)
#define SCL_PIN 5   // D1 pin on NodeMCU (GPIO5)

DHT11 dht(DHT_PIN);

uint8_t lcdAddress = 0x27;
LiquidCrystal_I2C lcd(lcdAddress, 16, 2);
bool lcdConnected = false;

ESP8266WebServer server(80);

void scanI2C() {
    Wire.begin(SDA_PIN, SCL_PIN);

    for(uint8_t addr = 0x20; addr <= 0x3F; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            lcdAddress = addr;
            lcdConnected = true;
            break;
        }
    }
}

bool initLCD() {
    if (!lcdConnected) return false;

    lcd = LiquidCrystal_I2C(lcdAddress, 16, 2);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    return true;
}

String getDHTData() {
    int temp, hum;
    int result = dht.readTemperatureHumidity(temp, hum);

    if (result != 0) {
        return "Error";
    }

    return String(temp) + "," + String(hum);
}

void setup(){
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    Serial.begin(115200);

    // 掃描並初始化LCD
    scanI2C();
    if (lcdConnected) {
        initLCD();
        Serial.println("LCD initialized");
    } else {
        Serial.println("LCD not found");
    }

    // 連接WiFi
    WiFi.begin(ssid, password);
    while(WiFi.status() != WL_CONNECTED){
        delay(1000);
        Serial.println("Connecting WiFi...");
    }

    Serial.println("WiFi connected");
    Serial.println(WiFi.localIP());

    // 路由設定
    server. on("/", HTTP_GET, [](){
        String html = "<!DOCTYPE html><html><head>";
        html += "<meta charset='UTF-8'>";
        html += "<title>ESP8266 控制器</title>";
        html += "<style>";
        html += "body{font-family:Arial;text-align:center;background:#f0f0f0;margin:40px;}";
        html += ". container{background:white;padding:30px;border-radius:10px;max-width:600px;margin:0 auto;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
        html += "h1{color:#333;margin-bottom:30px;}";
        html += "h2{color:#666;margin:25px 0 15px 0;}";
        html += "button{padding:10px 20px;margin:5px;border:none;border-radius:5px;cursor:pointer;font-size:14px;}";
        html += ". led-on{background:#4CAF50;color:white;}";
        html += ".led-off{background:#f44336;color:white;}";
        html += ".lcd-btn{background:#2196F3;color:white;}";
        html += ". dht-btn{background:#17a2b8;color:white;}";
        html += ". lcd-clear{background:#FF9800;color:white;}";
        html += "button:hover{opacity:0.8;}";
        html += "button:disabled{background:#ccc;cursor:not-allowed;}";
        html += "input{padding:8px;margin:5px;border:1px solid #ddd;border-radius:3px;width:200px;}";
        html += ".status{font-weight:bold;margin:10px 0;}";
        html += ".info{background:#f9f9f9;padding:15px;margin:20px 0;border-radius:5px;font-size:14px;}";
        html += ".sensor-data{background:#e8f5e8;padding:20px;margin:20px 0;border-radius:8px;}";
        html += ".temp-hum{display:inline-block;margin:0 15px;padding:10px;background:white;border-radius:5px;box-shadow:0 1px 3px rgba(0,0,0,0. 1);}";
        html += ".manual-display{color:#666;font-style:italic;padding:20px;}";
        html += "</style></head><body>";

        html += "<div class='container'>";
        html += "<h1>ESP8266 控制器</h1>";

        html += "<div class='info'>";
        html += "IP: " + WiFi. localIP().toString() + "<br>";
        html += "LCD 狀態: " + String(lcdConnected ? "Connected" : "Not Found");
        html += "</div>";

        // 溫濕度顯示區域
        html += "<div class='sensor-data'>";
        html += "<h2>溫濕度顯示區域</h2>";
        html += "<div id='dhtDisplay' class='manual-display'>按下可得到當前的數據</div>";
        html += "<button class='dht-btn' onclick='updateDHTData()'>顯示溫度、濕度</button>";
        html += "</div>";

        // LED控制
        html += "<h2>LED 控制</h2>";
        String ledStatus = digitalRead(LED_PIN) ? "ON" : "OFF";
        html += "<div class='status'>Status: <span id='ledStatus'>" + ledStatus + "</span></div>";
        html += "<button class='led-on' onclick=\"controlLED('on')\">開燈</button>";
        html += "<button class='led-off' onclick=\"controlLED('off')\">關燈</button>";

        // LCD控制
        html += "<h2>LCD 控制</h2>";
        html += "<button class='lcd-btn' onclick=\"controlLCD('show')\"" + String(lcdConnected ? "" : " disabled") + ">顯示 Hello World</button>";
        html += "<button class='lcd-clear' onclick=\"controlLCD('clear')\"" + String(lcdConnected ? "" : " disabled") + ">移除</button><br><br>";
        html += "<input type='text' id='customText' placeholder='Enter text'" + String(lcdConnected ? "" : " disabled") + "><br>";
        html += "<button class='lcd-btn' onclick=\"sendCustomText()\"" + String(lcdConnected ?  "" : " disabled") + ">傳送文字</button>";

        html += "</div>";

        // JavaScript
        html += "<script>";

        // 手動更新DHT數據
        html += "function updateDHTData(){";
        html += "  const display = document.getElementById('dhtDisplay');";
        html += "  display.innerHTML = 'Loading...';";
        html += "  fetch('/api/dht')";
        html += "    .then(response => response.json())";
        html += "    .then(data => {";
        html += "      if(data.error) {";
        html += "        display.innerHTML = '<div style=\"color:#f44336;\">DHT11 Sensor Error</div>';";
        html += "      } else {";
        html += "        display.innerHTML = ";
        html += "          '<div class=\"temp-hum\">Temperature: <strong>' + data.temperature + '°C</strong></div>' +";
        html += "          '<div class=\"temp-hum\">Humidity: <strong>' + data.humidity + '%</strong></div>';";
        html += "      }";
        html += "    })";
        html += "    .catch(error => {";
        html += "      display. innerHTML = '<div style=\"color:#f44336;\">Connection Error</div>';";
        html += "    });";
        html += "}";

        html += "function controlLED(action){";
        html += "  fetch('/led_'+action). then(r=>r.text()).then(d=>{";
        html += "    document.getElementById('ledStatus').innerText = action=='on'?'ON':'OFF';";
        html += "  });";
        html += "}";
        html += "function controlLCD(action){";
        html += "  fetch('/lcd_'+action);";
        html += "}";
        html += "function sendCustomText(){";
        html += "  const text = document.getElementById('customText').value;";
        html += "  if(! text. trim()) return;";
        html += "  fetch('/lcd_custom',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'text='+encodeURIComponent(text)})";
        html += "  .then(()=>document.getElementById('customText').value='');";
        html += "}";
        html += "</script></body></html>";

        server. send(200, "text/html", html);
    });

    // API端點獲取DHT數據
    server. on("/api/dht", HTTP_GET, [](){
        String dhtData = getDHTData();
        if (dhtData != "Error") {
            int commaIndex = dhtData.indexOf(',');
            String temp = dhtData. substring(0, commaIndex);
            String hum = dhtData.substring(commaIndex + 1);
            String json = "{\"temperature\":" + temp + ",\"humidity\":" + hum + "}";
            server. send(200, "application/json", json);
        } else {
            server. send(500, "application/json", "{\"error\":\"DHT11 read error\"}");
        }
    });

    server.on("/led_on", HTTP_GET, [](){
        digitalWrite(LED_PIN, HIGH);
        server.send(200, "text/plain", "LED ON");
    });

    server. on("/led_off", HTTP_GET, [](){
        digitalWrite(LED_PIN, LOW);
        server.send(200, "text/plain", "LED OFF");
    });

    server.on("/lcd_show", HTTP_GET, [](){
        if (!lcdConnected) {
            server.send(400, "text/plain", "LCD not connected");
            return;
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Hello World!");
        lcd.setCursor(0, 1);
        lcd.print("ESP8266 LCD");
        server. send(200, "text/plain", "Hello displayed");
    });

    server. on("/lcd_clear", HTTP_GET, [](){
        if (!lcdConnected) {
            server.send(400, "text/plain", "LCD not connected");
            return;
        }
        lcd.clear();
        server.send(200, "text/plain", "LCD cleared");
    });

    server.on("/lcd_custom", HTTP_POST, [](){
        if (!lcdConnected) {
            server.send(400, "text/plain", "LCD not connected");
            return;
        }
        if (server.hasArg("text")) {
            String text = server. arg("text");
            lcd.clear();

            if (text.length() <= 16) {
                lcd.setCursor(0, 0);
                lcd.print(text);
            } else {
                lcd.setCursor(0, 0);
                lcd.print(text.substring(0, 16));
                lcd.setCursor(0, 1);
                lcd. print(text.substring(16, 32));
            }
            server. send(200, "text/plain", "Text displayed");
        } else {
            server. send(400, "text/plain", "No text provided");
        }
    });

    server.begin();
    Serial.println("Server started");
}

void loop(){
    server.handleClient();
}