#include <MsTimer2.h>
#include <LiquidCrystal_I2C.h>
#include <pitches.h>
#include <Thread.h>
#include <ThreadController.h>

LiquidCrystal_I2C lcd(0x27, 16, 2);
const byte interruptPin = 2; //스위치 핀 번호
char ledPin = 4;
char soundPin = 8;

int unitTime = 500; //단점 1초, 장점 3초, 무음1초 - 점 구분, 무음3초 - 단어 구분, 무음 7초 - 문자열 구분
char count = 0; //푼 문제 개수
int correct = 0; //맞은 문제 개수
unsigned int Time = 0; //점 구분 시간
unsigned int lastTime = 0; //최근 받아온 시간
unsigned int debounceDelay = 50; //디바운스 기준 시간

boolean state_output; //오답시 정답을 출력할 센서 선택 화면 플래그
boolean state_mode = false; //센서 선택, false : LED, true : 피에조 부저
boolean state_led = false; //센서 작동 플래그
boolean state_power; //전원 상태 플래그
boolean state_clear = false;  //lcd 화면 초기화 플래그
boolean state_result; //정답 화면 송출 플래그
boolean state_result2;  //전체 결과 화면 송출 플래그
boolean state_home; //홈 화면 플래그
boolean state_level; //레벨 설정 플래그
boolean state_init_word;//단어 초기화 플래그
boolean pressed = false; //스위치 상태 플래그

char QUIZ_SEC = 15; //퀴즈 타이머 최대 길이

// ThreadController that will controll all threads
ThreadController controll = ThreadController();
//RemainSecPrint Thread
Thread printRemainSecThread = Thread();
Thread runLEDThread = Thread();

//단어 리스트, 하급 난이도 문제로 사용
char wordList[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                   'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'
                  };
String words = ""; //누적된 숫자
String str1 = ""; //lcd 1행
String str2 = ""; //lcd 2행
String str3 = ""; //입력된 문자열


void initStates() { //상태 초기화 함수
  state_power = true;
  state_result = false;
  state_result2 = false;
  state_home = true;
  state_init_word = false;
  str3 = "";
}

class Quiz { //문제 제출 클래스
  private:
    int level; //문제 난이도 선택 변수
    int quizSec; //타이머 시간
    String high_quiz[7] = {"DOG", "CAT", "PIG", "COW", "DUCK", //상급 난이도 문제 리스트
                           "HIPPO", "MONKEY"
                          };
  public:
    Quiz() {
      quizSec = QUIZ_SEC; //타이머 시간을 초기화
    }
    int getLevel() {
      return level;
    }
    int getQuizSec() {
      return quizSec;
    }

    void setLevel(int quizLevel) {
      level = quizLevel;
    }

    void decreaseQuizSec() {
      quizSec -= 1;
    }

    void initSec() {
      quizSec = QUIZ_SEC;
    }


    char getLowQuiz() {//하급 난이도 문제 추출
      randomSeed(analogRead(A0));
      int index = random(sizeof(wordList) / sizeof(char));
      return wordList[index];
    }

    String getHighQuiz() {//상급 난이도 문제 추출
      randomSeed(analogRead(A0));
      int index = random(sizeof(high_quiz) / sizeof(String));
      return high_quiz[index];
    }
};

Quiz quiz;

void printRemainSec() { //퀴즈 타이머 출력
  int quizSec = quiz.getQuizSec();
  lcd.setCursor(14, 0); //우측 상단
  if (quizSec < 10) {
    lcd.print("0");
    lcd.setCursor(15, 0);
  }
  lcd.print(String(quizSec));

  // 남은 시간 감소
  quiz.decreaseQuizSec();

  if (quizSec < 0) {  //시간 내에 못 풀었을 경우
    controll.remove(&printRemainSecThread);//퀴즈 타이머 쓰레드 제거
    setStrings(getString(1), "Fail"); //실패
    state_result = true; //결과 화면 플래그 설정
    count++; //문제 카운트 증가
    //--------------------------------------------led 호출 ----------------------------
    controll.add(&runLEDThread);
  }
}



void runLED() {//오답 입력시 정답을 알려주는 기능을 하는 함수
  String dotList[] = {"12", "2111", "2121", "211", "1", "1121", "221", "1111", "11",
                      "1222", "212", "1211", "22", "21", "222", "1221", "2212", "121",
                      "111", "2", "112", "1112", "122", "2112", "2122", "2211", "22222",
                      "12222", "11222", "11122", "11112", "11111", "21111", "22111", "22211", "22221"
                     };
  bool ledStatus = false; //led, 피에조 부저 on, off 플래그,
  static int num = 0 ;  //센서를 출력할 길이
  static int i = 0; //퀴즈의 글자를 한글자씩 가져오기위한 인덱스
  static int j = 0; //가져온 글자의 점리스트를 한개씩 가져오기위한 인덱스
  if (!state_led) {
    i = 0;
    j = 0;
    state_led = true;
  }

  if (num == 0) {
    if (i > str1.length()) {
      controll.remove(&runLEDThread);
      state_led = false;
      return;
    }
    str3 = dotList[str1[i] - 'A'];
    if (str3[j] == '1') {
      num = 1;
    } else if (str3[j] == '2') {
      num = 3;
    } else {
      num = -1;
      j = -1;
      i++;
    }
    j++;
  } else if (num > 0) {
    ledStatus = true;
    num--;
  } else {
    num++;
  }
  if (!state_mode) {
    digitalWrite(ledPin, ledStatus);
  } else {
    if (ledStatus) {
      tone(soundPin, NOTE_G5);
    } else {
      noTone(soundPin);
    }
  }
}





void startStudy() { //문제 맞추기 시작
  state_level = false; //설정 화면 탈출
  state_clear = true; //lcd 초기화
  count = 0;
  correct = 0;
  //문제 출력
  makeQuiz();
}

//난이도 설정
void selectLevel() {
  str3 = "";
  state_output = false;
  state_level = true;
  state_clear = true;
  setStrings("Level", "Low:o,High:-");
}

void selectOutput() { //출력 센서 선택
  str3 = "";
  state_home = false;
  state_output = true;
  state_clear = true;
  setStrings("Select Output", "LED:o,Sound:-");
}

void makeQuiz() {
  controll.remove(&runLEDThread);
  digitalWrite(ledPin, false);
  noTone(soundPin);
  state_led = false;
  state_clear = true;
  if (quiz.getLevel() == 0)
    str1 = quiz.getLowQuiz();
  else
    str1 = quiz.getHighQuiz();
  str3 = "";
  quiz.initSec();
  controll.add(&printRemainSecThread); //퀴즈 타이머 쓰레드 실행
}

void setStrings(String mStr1, String mStr2) {//lcd에 출력할 문자열 설정 함수
  state_clear = true;
  str1 = mStr1;
  str2 = mStr2;
}

String getString(int num) {
  switch (num) {
    case 1:
      return str1;
    case 2:
      return str2;
    case 3:
      return str3;
    default:
      return "";
  }
}

// ------------------------------ 단어 매핑 --------------------------------------

String mapWord() { //누적된 숫자를 문자로 변경하는 함수
  String temp = "";  //누적 입력 리스트에 추가할 임시 변수
  //모스부호 변환 리스트, 단점 = 1, 장점 = 2
  String dotList[] = {"12", "2111", "2121", "211", "1", "1121", "221", "1111", "11",
                      "1222", "212", "1211", "22", "21", "222", "1221", "2212", "121",
                      "111", "2", "112", "1112", "122", "2112", "2122", "2211", "22222",
                      "12222", "11222", "11122", "11112", "11111", "21111", "22111", "22211", "22221"
                     };
  for (int i = 0; i < sizeof(wordList) / sizeof(char); i++) {
    if (words.equals(dotList[i])) {
      temp = (String)wordList[i];
      break;
    }
  }
  words = "";
  return temp;
}

void stopInput() { //3초간 미입력시 입력 중지 및 입력 데이터 처리
  str3 += mapWord();
  if (state_home) { //홈 화면일 경우
    //str3 += mapWord();
    if (str3.equals("S")) { //퀴즈 스타트
      selectOutput();
    } else if (str3.equals("E")) {//시스템 종료
      state_power = false;
    } else {
      str3 = "";
      //------------------------- 오류 음 추가 -----------------------------
      tone(soundPin, NOTE_G5, 125);
    }
  } else if (state_level) {//난이도 선택 화면일 경우
    //str3 += mapWord();
    if (str3.equals("E")) {
      //난이도 쉽게
      quiz.setLevel(0);
      startStudy();
    } else if (str3.equals("T")) {
      //난이도 어렵게
      quiz.setLevel(1);
      startStudy();
    } else {
      str3 = "";
    }
  } else if (state_output) {//센서 선택화면일 경우
    if (str3.equals("E")) {
      //led로 출력
      state_mode = false;
      selectLevel();
    } else if (str3.equals("T")) {
      //sound로 출력
      state_mode = true;
      selectLevel();
    } else {
      str3 = "";
    }
  } else {//결과 화면일 경우
    controll.remove(&printRemainSecThread);
    state_result = true;
    //str3 += mapWord();
    if (str3.equals(str1)) {
      setStrings(getString(1), "Sucess");
      correct++;
    } else {//오답일 경우
      setStrings(getString(1), "Fail");
      controll.add(&runLEDThread);
    }
    //문제 개수 증가
    count++;
    state_init_word = true;

  }
  MsTimer2::stop();
}


// -------------------------------- 스위치 인터럽트 로직 -------------------------------------

void processState() {         //스위치 입력 처리
  Time = millis() - lastTime; //디바운스 및 단음, 장음 구분
  digitalWrite(ledPin, false);
  noTone(soundPin);
  if (Time > debounceDelay) { //디바운스
    MsTimer2::stop(); //타이머 재시작을 위한 정지
    if (!state_power) {       //전원이 꺼져있으면
      if (pressed) {  //버튼이 눌려져있으면 = 버튼을 뗄 때, 없으면 2번 동작하여 타이머를 작동시킴
        initStates();
        setStrings("Menu", "Start:ooo,End:o");          //홈 화면 출력
        pressed = false;      //!pressed로 할 경우 간혹 디바운싱으로 인한 반전 버그 발생
      } else {
        pressed = true;
      }
    } else if (state_result) {           //전체 결과 화면이면
      if (pressed) {
        if (count >= 5) {                //5문제를 풀었을 경우
          state_result2 = true;

          String temp = (String)correct + " Correct!";
          setStrings("5 Quizzes", temp);
        } else {                         //5문제를 다 풀지 않은 경우
          makeQuiz();
        }
        state_result = false;
        pressed = false;
      } else {
        pressed = true;
      }
    } else if (state_result2) {          //전체 결과 화면인 경우 다시 홈으로 돌아감
      if (pressed) {
        controll.remove(&runLEDThread);
        state_led = false;
        state_clear = true;
        setStrings("Menu", "Start:ooo,End:o");
        state_home = true;
        state_result2 = false;
        pressed = false;
      } else {
        pressed = true;
      }
    } else {                              //스위치 인터럽트 처리
      if (!pressed) {                     //버튼을 누를 때(무음)
        if (Time > unitTime) {            //장무음일 경우
          str3 += mapWord();              //알파벳 하나가 추가
        } else {                          //단무음일 경우
        }
        pressed = true;                   //버튼을 누른 상태

      } else {                            //버튼을 뗄 때
        if (Time > unitTime) {            //장점일 경우
          words += "2";                   //누적 숫자 리스트에 2 추가
        } else {                          //단점일 경우
          words += "1";                   //누적 숫자 리스트에 1 추가
        }
        pressed = false;                  //버튼을 뗀 상태
        MsTimer2::start();                //타이머 재시작
        Serial.println("타이머 스타트");
      }

    }
  }
  lastTime = millis();                    //최근 시간 재설정
}




void setup() {
  // put your setup code here, to run once:
  pinMode(interruptPin, INPUT_PULLUP);    //스위치 핀 등록
  Serial.begin(9600);                     //시리얼 등록
  Serial.println("test start");
  initStates();
  MsTimer2::set(unitTime * 3, stopInput); //단어 구분 타이머 등록
  attachInterrupt(digitalPinToInterrupt(interruptPin), processState, CHANGE); //스위치 인터럽트 등록
  setStrings("Menu", "Start:ooo,End:o");  //홈 화면 문자열 등록
  lcd.begin();                            //lcd 등록
  lcd.backlight();                        //조명 on
  printRemainSecThread.onRun(printRemainSec); //퀴즈 타이머 쓰레드 등록
  printRemainSecThread.setInterval(1000); //1초에 한번씩 작동

  runLEDThread.onRun(runLED); //정답 출력 센서 쓰레드 등록
  runLEDThread.setInterval(300);
  // Adds myThread to the controll

  pinMode(ledPin, OUTPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!state_power) {//시스템을 종료할 경우
    lcd.clear();
    lcd.noBacklight();
    lcd.noDisplay();
  } else {//시스템을 켤 경우
    lcd.display();
    lcd.backlight();
  }

  if (state_clear) {//lcd화면을 지우는 경우
    lcd.clear();
    state_clear = false;
  }

  lcd.setCursor(0, 0); //1행
  lcd.print(str1); //랜덤 문자 혹은 단어 출력
  lcd.setCursor(0, 1); //2행
  if (state_home || state_result || state_result2 || state_level || state_output) { //결과 화면은 지정된 문자열 출력
    lcd.print(str2); //

  } else {
    lcd.print(str3); //문제 화면은 사용자 입력 단어
  }
  if (state_init_word) { //이벤트 메서드에서 바로 문자열 초기화시 loop문에서 lcd가 읽지 못함
    str3 = "";
    state_init_word = false;
  }
  controll.run();   //쓰레드 실행
}
