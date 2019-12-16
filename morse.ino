#include <MsTimer2.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
const byte interruptPin = 2; //스위치 핀 번호

int unitTime = 1000; //단점 1초, 장점 3초, 무음1초 - 점 구분, 무음3초 - 단어 구분, 무음 7초 - 문자열 구분
unsigned long Time = 0; //점 구분 시간
unsigned long lastTime = 0; //최근 받아온 시간
unsigned long debounceDelay = 50; //디바운스 기준 시간

boolean state = false; //누른 상태, 뗀 상태 구분

//모스부호 변환 리스트, 단점 = 1, 장점 = 2
String dotList[] = {"12","2111","2121","211","1","1121","221","1111","11", 
                  "1222","212","1211","22","21","222","1221","2212","121",
                  "111","2","112","1112","122","2112","2122","2211","22222",
                  "12222","11222","11122","11112","11111","21111","22111","22211","22221"};
//단어 리스트
String wordList[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
                   "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
                   "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
String words = ""; //누적된 숫자
String str1 = "";
String str2 = ""; //입력된 문자열

String mapWord() { //누적된 숫자를 문자로 변경하는 함수
  String temp = "";
  Serial.println("시작");
  for(int i =0; i<sizeof(wordList)/sizeof(String); i++) {
    if(words.equals(dotList[i])) {
      temp = wordList[i];
      break;
    }
  }
  if(temp == "") {
    Serial.println("일치하는 단어가 존재하지 않습니다.");
  } else {
    Serial.println(temp);
  }
  words = "";
 
  return temp;
}

void stopInput() { //3초간 미입력시 입력 중지 및 문자열 출력
  str2 += mapWord();
  Serial.println(str2);
  str2="";
  state=false; //간혹 버그로 인한 점과 무음의 반전 현상 후 처리
  MsTimer2::stop();
}


void processState() { //스위치 입력 처리
  MsTimer2::stop(); //스위치에 변화가 있을때마다 타이머 재시작
  Time = millis() - lastTime;
  if(Time > debounceDelay) { //디바운스
    Serial.print("상태 : ");
    Serial.print(state);
    Serial.print(",    ");
    if(!state) { //버튼을 누를 때
      if(Time > unitTime) {
        Serial.print("장무음  ");
        Serial.println(Time);
        str2 += mapWord();
      } else {
        Serial.print("단무음  ");
        Serial.println(Time);
      }
    } else { //버튼을 뗄 때
      if(Time > unitTime) {
        Serial.print("장점  ");
        Serial.println(Time);
        words += "2";
      } else {
        Serial.print("단점  ");
        Serial.println(Time);
        words += "1";
      }
      
    }
    state = !state; //무음, 점 반전
    lastTime = millis(); //장, 단을 구분하기 위한 시간 재설정
  }
    MsTimer2::start(); //타이머 재시작
}



void setup() {
  // put your setup code here, to run once:
  pinMode(interruptPin, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("test start");
  MsTimer2::set(unitTime*3, stopInput);
  attachInterrupt(digitalPinToInterrupt(interruptPin), processState, CHANGE);
  lcd.begin();
  lcd.backlight();
  //attachInterrupt(digitalPinToInterrupt(interruptPin), initState, FALLING);
  //attachInterrupt(digitalPinToInterrupt(interruptPin), processState, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:
  lcd.setCursor(0,0); //1행
  lcd.print("Hi"); //랜덤 문자 혹은 단어 출력
  lcd.setCursor(0,1); //2행
  lcd.print(str2); //사용자 입력 단어
}
