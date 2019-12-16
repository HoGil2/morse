
const byte interruptPin = 2;

int unitTime = 1000; //단점 1초, 장점 3초, 무음1초 - 점 구분, 무음3초 - 단어 구분, 무음 7초 - 문자열 구분
unsigned long Time = 0; //점 구분 시간
unsigned long lastTime = 0; //최근 받아온 시간
boolean state = false;
//String

//단점 = 1, 장점 = 2
int dotList[] = {12,2111,2121,211,1,1121,221,1111,11,
                  1222,212,1211,22,21,222,1221,2212,121,
                  111,2,112,1112,122,2112,2122,2211,22222,
                  12222,11222,11122,11112,11111,21111,22111,22211,22221};
char wordList[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                   'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

void showWord() {
  Serial.println("Word!");
  Serial.println("");
}

void processState() {
  Time = millis() - lastTime;
  Serial.print("상태 : ");
  Serial.print(state);
  Serial.print(",    ");
  if(!state) {
    if(Time > unitTime) {
      Serial.print("장무음  ");
    } else {
      Serial.print("단무음  ");
    }
  } else {
    if(Time > unitTime) {
      Serial.print("장점  ");
    } else {
      Serial.print("단점  ");
    }
  }
  state = !state;
  Serial.println(Time);
  lastTime = millis();
}



void setup() {
  // put your setup code here, to run once:
  pinMode(interruptPin, INPUT_PULLUP);
  Serial.begin(9600);
  Serial.println("test start");
  attachInterrupt(digitalPinToInterrupt(interruptPin), processState, CHANGE);
  //attachInterrupt(digitalPinToInterrupt(interruptPin), initState, FALLING);
  //attachInterrupt(digitalPinToInterrupt(interruptPin), processState, RISING);
}

void loop() {
  // put your main code here, to run repeatedly:

}
