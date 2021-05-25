// khai báo thư viện
#include "MAX30100_PulseOximeter.h"
#include <Keypad.h> 
#include <U8g2lib.h>
  
// khai báo tham số cần thiết
#define REPORTING_PERIOD_MS     1000

// khai báo biến thư viện u8g2 của st7920
U8G2_ST7920_128X64_F_SW_SPI u8g2(U8G2_R0, /* clock=*/ 13, /* data=*/ 11, /* CS=*/ 10, /* reset=*/ 8);

// khai báo biến pox là biến sử dụng để đo nhịp tim và oxi trong máu
PulseOximeter pox;
MAX30100 max30100;

// biến thời gian
uint32_t tsLastReport = 0;

// khai báo biến counter_beat
// khi biến counter_beat = 10 thì hiển thị ra màn hình
int counter_beat = 0;

// Callback (registered below) fired when a pulse is detected
// hàm này được gọi khi xuất hiện xung nhịp tim
void onBeatDetected()
{
    Serial.print("Beat! ->");
    Serial.println(counter_beat);
}

// KHAI BÁO CÁC THAM SỐ CẦN CHO BÀN PHÍM
const byte ROWS = 4; //four rows
const byte COLS = 3; //four columns
//define the cymbols on the buttons of the keypads
char hexaKeys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {2, 3, 4, 5}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {6, 7, 9}; //connect to the column pinouts of the keypad

//initialize an instance of class NewKeypad
Keypad customKeypad = Keypad( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS); 

int counter_point = 30; // biến đếm để nhảy con trỏ hiển thị passworld
int i = 0; // biến đếm trong mảng
bool checkPass = false;
char key;
int lengthPass = 4;
char enter_pass[4] = {' ',' ',' ',' '};   // mảng chứa pass nhập vào

// hiện thông báo nhập pass
void fn_notice_EnterPass(){
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.setCursor(0,10);
      u8g2.println("Welcome to Heart Rate!");
      u8g2.setCursor(0,20);
      u8g2.println("Your Code:");
      u8g2.sendBuffer();
}

// thao tác nhập pass
void fn_EnterPass() {
  // kiểm tra nếu có nhấn phím thì in ra và tăng lên 1, lặp lại hành động này
  for(; i < lengthPass ; i = i + 1) {
    key = customKeypad.getKey();
    if(key) {
      u8g2.setCursor(counter_point, 30);  // nhảy con trỏ đến vị trí hiển thị số
      u8g2.print(key);  // in số ra màn hình
      enter_pass[i] = key;  // lưu key vào mảng 
      u8g2.sendBuffer(); // gửi lênh in ra màn hình
      delay(100);
      u8g2.setCursor(counter_point, 30);
      u8g2.print("*");
      u8g2.sendBuffer();
      counter_point = counter_point + 8;  // tăng vị trí con trỏ nhảy
    }
    else i = i-1;  
  }
  counter_point = 30;
}

void showLoading() {
  u8g2.clearBuffer();
  u8g2.setCursor(2,24);
  u8g2.print("Loading...");
  u8g2.sendBuffer();
}

// kiểm tra pass 
bool fn_CheckPass() {
  // tuyền dữ liệu từ arduino sang esp
  String sendPass = "pass-";
  if(i >= lengthPass) {
    for(int counter = 0; counter < lengthPass; counter = counter + 1) {
      sendPass += enter_pass[counter];
    }
    // gửi dữ liệu
    Serial3.println(sendPass);
  }
  Serial3.flush();

  showLoading();  // hiển thị màn hình loading trong khi chờ phản hồi từ esp

    // đọc dữ liệu phản hồi lại từ esp
    if(i >= lengthPass) { 
      String str = "";
      while(!Serial3.available());   // chờ cho đến khi esp phản hồi về
      str = Serial3.readString();    // đọc chuỗi phản hồi về từ esp
      Serial.println(str);           
      delay(1000);
      if(str == "correct\r\n") return true;   // nếu pass đúng thì chuỗi nhận về là correct
    }
  return false;
}

// đo nhịp tim và oxi trong máu , đo đủ 10 lần thì dừng
void fn_measure_Heart_Oxi() {
  pox.begin();
  while(counter_beat < 10) {
   pox.update();
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
      counter_beat = counter_beat + 1;
      
      Serial.print(pox.getHeartRate());
      Serial.print("pbm/ sp2:");
      Serial.println(pox.getSpO2());    

      tsLastReport = millis();
    }
  }
}

// hiện thông tin lên màn hình
void fn_Show_Infor() {
    String sendValueHeartRate = "sensor-";
    pox.update();
    u8g2.clearBuffer();
    u8g2.setCursor(1,10);
    u8g2.print("Heart: ");
    u8g2.setCursor(1,20);
    u8g2.print(pox.getHeartRate());
    sendValueHeartRate = sendValueHeartRate + (int)pox.getHeartRate() + "-";
    delay(500);
    
    u8g2.print("bpm/Sp02:");
    u8g2.print(pox.getSpO2());
    sendValueHeartRate += (int)pox.getSpO2();
    u8g2.println("%");
    u8g2.setCursor(50,40);
    u8g2.print("Next");
    u8g2.sendBuffer();
    do {
      key = customKeypad.getKey();
      if(key) break;
    }while(true);
    showLoading();
    counter_beat = 0; 
    pox.shutdown(); 
    Serial3.println(sendValueHeartRate);
    Serial3.flush(); 
    while(!Serial3.available());   // chờ cho đến khi esp phản hồi về
    String str = Serial3.readString();    // đọc chuỗi phản hồi về từ esp
    if(str != "Error\r\n") {
      showRecommend(str);
    }
}

// hàm hiển thị nội dung Recommend
void showRecommend(String str) {
  u8g2.clearBuffer();
  int j=0, k=0, lenRow = 18;
  String strNew = "";
  String w = "";
  String a[20] = {"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "","", "", ""};
  for(int i = 0; i < str.length(); i++) {
    if(j < lenRow) {
      strNew += str[i];
      j++;
    }
    else if(j == lenRow && str[i+1] != ' ') {
      while(strNew[j] != ' ') {
        i--;
        j--;
      }
      j++;
      strNew.remove(j);
      a[k] = strNew;
      strNew = "";
      j = 0;
      k++;
    }
    else {
      strNew += str[i];
      a[k] = strNew;
      strNew = "";
      j = 0; 
      k++;
    }
  }
  for(int i = 0; i <= k; i++) {
    Serial.println(a[i]);
  } 
  showRecommendChild(0,a, k +1);  
}

void showRecommendChild(int k, String a[], int lenArr) {
  u8g2.clearBuffer();
  int key;
  int pos = 10;
  if(k >= lenArr ) return;
  for(int i = 0; i< 4 && k < lenArr; i++) {
    u8g2.setCursor(0,pos);
    pos += 10;
    u8g2.println(a[k]);
    Serial.println(a[k]);
    k +=1;
  }
  u8g2.setCursor(50,pos);
  u8g2.println("Next");
  u8g2.sendBuffer();
  do{
    key = customKeypad.getKey();
    if(key) {
      showRecommendChild(k, a, lenArr);
      return;
    }
  }while(true);
}

// kiểm tra user có muốn tiếp tục đo ko?
bool fn_Check_Continue() {
  u8g2.clearBuffer();
  u8g2.setCursor(1, 10);
  u8g2.println("Keep Measuring?");
  u8g2.setCursor(1,20);
  u8g2.print("*->Yes/ #->No: ");
  u8g2.sendBuffer();
  do {
    key = customKeypad.getKey();
    if(key) {
      u8g2.print(key);
      u8g2.sendBuffer();
      break;
    }
  }while(true);
  if(key == '*') return true;
  else if(key == '#') return false;
}

// thiết lập ban đầu
void setup()
{
  u8g2.begin();
  // mở cổng giao tiếp
  Serial.begin(115200);
  Serial.begin(9600);
  Serial3.begin(9600);

  u8g2.setFont(u8g2_font_ncenB08_tr);

  pox.setOnBeatDetectedCallback(onBeatDetected);
  if(pox.begin()) pox.shutdown();
}

void loop()
{   
    Serial.println("Start Device");
    if(!checkPass && i < lengthPass) {
      // nếu checkPass = false thì hiện thông báo nhập pass
      u8g2.clearBuffer();
      fn_notice_EnterPass();
      
      if(customKeypad.waitForKey()) {
        // nêu có event nhấn phím thì cho nhập pass
        fn_EnterPass();  
        // kiểm tra pass
        checkPass = fn_CheckPass();
        Serial.println(checkPass);
      }
    }
    if(checkPass && i >=lengthPass) {
     // nếu pass đã nhập là đúng thì thực hiện đo
     //delay(1000);
     u8g2.clearBuffer();
     u8g2.setCursor(2,24);
     u8g2.print("Starting Measure!");
     u8g2.sendBuffer();
      fn_measure_Heart_Oxi();
      // hiện thông tin đo lên màn hình
      fn_Show_Infor();
      // kiểm tra user đó có muốn đo tiếp ko?
      // nếu ko thì gán checkPass = false để thực hiện nhập pass lại từ đầu
      if(!fn_Check_Continue()) {
        checkPass = false;
        i = 0; 
      }
    }

    if(!checkPass && i >=lengthPass) {
      u8g2.clearBuffer();
      u8g2.setCursor(2, 24);
      u8g2.print("InCorrect Password!");
      u8g2.sendBuffer();
      i = 0;
    }
    delay(1000);
}
