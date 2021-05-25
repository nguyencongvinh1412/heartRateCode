#include <SoftwareSerial.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define RX D1
#define TX D2
SoftwareSerial mySerial(RX, TX);

const char* ssid = "Cong Vinh";
const char* password = "0327949281";

String serverName = "https://heart-rate4124.000webhostapp.com/Server";
unsigned long lastTime = 0;
unsigned long timerDelay = 1000;

void setup() {
  // put your setup code here, to run once:
  mySerial.begin(9600);
  Serial.begin(9600);
  
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if(mySerial.available()) {
    String key = "";
    String data = "";
    bool checkkey = false;
    while(mySerial.available()) {
      // sensor-66-96\r\n
      // pass-123456789\r\n
      char c = mySerial.read();
      if(c == '\r') continue;
      else if(c == '\n') break;
      else if(c != '-' && checkkey == false) key += c;
      else if(c == '-' && checkkey == false) checkkey = true;
      else data += c;
    }

    // kiểm tra request có thành công hay không
    if(http_request(data, key)) {
      // nếu thực hiện request check_pass
      if(key == "pass") {
        String str =  receiveResponse(key);
        if(str.length() > 0) {
          mySerial.println("correct");
          Serial.println(str);
        }
        else {
          mySerial.println("Incorrect");
        }
      }
      // nếu thực hiện xử lý dữ liệu cảm biến
      else if(key == "sensor") {
        String str =  receiveResponse(key);
        if(str.length() > 0) {
          mySerial.println(str);
          Serial.println(str);
        }
        else {
          mySerial.println("Error");
          Serial.println("Lỗi recommend");
        }
      }
    }
  }
  delay(2000);
}

bool http_request(String data, String keyRequest) {
  if ((millis() - lastTime) > timerDelay) {
    if(WiFi.status()== WL_CONNECTED){
      HTTPClient http;
      String url = serverName;
      // hành vi kiểm tra pass
      if(keyRequest == "pass") {
        url = url + "/check_pass.php?pass=" + String(data);
        http.begin(url, "5bfbd1d449d30fa9c6400334bae02405aad2e201");      
      }
      
      // hành vi chẩn đoán dữ liệu
      else {
        // tách chuỗi dữ liệu chứa giá trị nhịp tim và oxi trong máu
        // vd : 66-96
        // tách thành:
        // heart_rate = 66
        // oxi_meter = 96
        int index = 0;
        String heart_rate = "";
        String oxi_meter = "";
        bool checkSensor = false;
        Serial.println(data);
        while(index < data.length()) {
            if(data[index] != '-' && checkSensor == false) heart_rate += data[index];
          else if(data[index] != '-' && checkSensor == true) oxi_meter += data[index];
          else checkSensor = true;
          index = index +1;
        }

        Serial.println(heart_rate + '&' + oxi_meter);
        url = url + "/training.php?heart=" + String(heart_rate) + "&oximeter=" + String(oxi_meter);
        http.begin(url, "5bfbd1d449d30fa9c6400334bae02405aad2e201");
      }
      
        int httpResponseCode = http.GET();
        if (httpResponseCode == 200) {
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          http.end();
          return true;
        }
        else {
          Serial.print("Error code: ");
          Serial.println(httpResponseCode);
          http.end();
          return false;
        }
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
  return false;
}

String receiveResponse(String key)
{
  String lastmessage="";
 if(WiFi.status()==WL_CONNECTED)
  {
    HTTPClient http;
    if(key == "pass") {
      String url="https://heart-rate4124.000webhostapp.com/Server/dataUser.txt";
      http.begin(url, "5bfbd1d449d30fa9c6400334bae02405aad2e201");
    }

    else if(key == "sensor") {
      String url="https://heart-rate4124.000webhostapp.com/Server/dataRecommend.txt";
      http.begin(url, "5bfbd1d449d30fa9c6400334bae02405aad2e201");
    }
    
    http.addHeader("Content-Type","text/plain");
    int httpCode=http.GET();  // trả về 200 có nghĩa là get dữ liệu thành công
    String data=http.getString();
    lastmessage=data;
    http.end();
  }
  else
  {
   lastmessage="";
  }
  return lastmessage;
}
