/* Omni Motor Control ByU Robot

  작성자: youngi-gi kim, Byeong-ho Lee

  참고자료: https://github.com/lupusorina/nexus-robots : NEXUS_ROBOT demo-code 및 Library
           https://simsamo.tistory.com/13?category=700958 : filter

*/

#define _NAMIKI_MOTOR
#include <MatrixMath.h>
#include <math.h>

// 초음파 pin 정의
#define TRIG_Front 24//정면 초음파 TRIG pin
#define ECHO_Front 25//정면 초음파 ECHO pin

#define TRIG_Left 26//좌측 초음파 TRIG pin
#define ECHO_Left 27//좌측 초음파 ECHO pin

#define TRIG_Right 28//우측 초음파 TRIG pin
#define ECHO_Right 29//우측 초음파 ECHO pin

#define TRIG_SIDE_L 30//좌측 측면 초음파 TRIG pin
#define ECHO_SIDE_L 31//좌측 측면 초음파 ECHO pin

#define TRIG_SIDE_R 32//우측 측면 초음파 TRIG pin
#define ECHO_SIDE_R 33//우측 측면 초음파 ECHO pin

// 초음파 측정 최대 거리: PulseIn함수에서 측정거리 over될 시 0을 반환
#define DIST_MAX 150 // 최대거리 일때 반환 0이면 150으로 출력
#define DIST_MIN 2 // 측정 최소 거리 지정
#define DIST_S (DIST_MAX *58.2) //초음파센서 타임아웃 설정 (최대거리)


// Motor Pin 정의부

// 뒷 바퀴 1번
#define BACK_PWM 13
#define BACK_DIR 12
#define BACK_ENCODER_A 2
#define BACK_ENCODER_B 3

// 왼쪽바퀴 2번
#define LEFT_PWM 9
#define LEFT_DIR 8
#define LEFT_ENCODER_A 21
#define LEFT_ENCODER_B 20

// 오른쪽 바퀴 3번
#define RIGHT_PWM 7
#define RIGHT_DIR 6
#define RIGTH_ENCODER_A 19
#define RIGTH_ENCODER_B 18

// Omni Body 관련 정의부
#define NR_OMNIWHEELS 3 // 옴니휠 갯수
#define BODY_RADIUS  160 //mm 로봇 반지름
#define REDUCTION_RATIO_NAMIKI_MOTOR 80 //감속비
#define WHEEL_RADIUS  24 //바퀴 반지름
#define WHEEL_CIRC  (WHEEL_RADIUS * 2 * M_PI) //M_PI는 math.h에 있는 pi값. 바퀴 둘레값

// Speed Parameter
#define  MAX_SPEEDRPM 8000 //최대 속도RPM
#define  MAX_PWM 255  //PWM 값

// Motor Speed, DIR
float back_wheel_speed, right_wheel_speed, left_wheel_speed;
float matrix_w_b[3][3];
float matrix_b_w[3][3];
int i, j;
bool backdir;
bool leftdir;
bool rightdir;

// Encoder Position
float BackencoderPos = 0; //현재 모터 회전수
float LeftencoderPos = 0;
float RightencoderPos = 0;

// 초음파 timer
unsigned long cur_time ;
unsigned long pre_time ;

const long utime = 2500; // timer loop time: 2500us
const long mtime = 50; // timer loop time: 50ms

//초음파 alpha filter
unsigned long  pre_distance_F;
unsigned long  pre_distance_L;
unsigned long  pre_distance_R;

unsigned long  pre_distance_Side_L;
unsigned long  pre_distance_Side_R;

const float alpha = 0.999;
const float beta = (1 - alpha);

//필터 적용된 distance
unsigned long  distance_F;
unsigned long  distance_L;
unsigned long  distance_R;

unsigned long  distance_Side_L;
unsigned long  distance_Side_R;

//////////////////////////////////////
////// Setup ////////////////
void setup() {
  // put your setup code here, to run once:
  //Serial.begin(9600);

  setInterruptpin(); //인터럽트 핀 설정
  SetupMotorPin();  // Motor Pin 설정
  SetupUltraPin();  // Ultra Pin 설정

  generate_matrix_wheel_body(matrix_w_b, matrix_b_w);

  drive_line_body_frame(-150, 0, 0, 0);  // 정상주행 앞으로

}

unsigned long s_time = 0; // 루프 타임체크용
unsigned long e_time = 0; // 루프 타임체크용

//////////////////////////////////////////////////////
////////  Loop //////////////////////////////
void loop() {
  // put your main code here, to run repeatedly:

  cur_time = millis();

  if (cur_time - pre_time >= mtime) // 50ms 마다 초음파 측정
  {
    //s_time = micros();
    Read_distance();
    //ShowDistance();
    Collision_Avoidance();
    //e_time = micros();
    //Serial.println(((e_time - s_time) / 1000));
    pre_time = cur_time ;
  }

}

void Collision_Avoidance() {

  if (distance_F <= 40) { // 장애물이 감지되면
    drive_line_body_frame(0, 0, 0, 0); //속도감소, 일단 멈춤
   
    if (distance_F <= 30) {  // 정면이 막혔을 때
      if (distance_L <= 30 && distance_R <= 30) { //좌우가 다 막히면, U턴
        drive_line_body_frame(0, 0, 20, 1000);
      }
      else if (distance_L <= 30) { //좌측 막히면 우측으로 회전
        drive_line_body_frame(0, 0, 5, 500);
      }
      else if (distance_R <= 30) { //우측 막히면 좌측으로 회전
        drive_line_body_frame(0, 0, -5, 500);
      }
      if (distance_Side_L <= 20) { // 앞이 막혀서 회전 한 후에 옆에 장애물이 가까이 있을 경우 거리를 두도록 한다.
        drive_line_body_frame( 0, 200, 0, 200);
        if ( distance_Side_R <= 20) { // 옆으로 움직였는데 반대쪽 장애물에 가까워진 경우 반대로 다시 살짝 움직여준다.
          drive_line_body_frame( 0, -100, 0, 200);
        }
      }
      else if (distance_Side_R <= 20) {
        drive_line_body_frame( 0, -200, 0, 200);
        if ( distance_Side_L <= 20) {
          drive_line_body_frame( 0, 100, 0, 200);
        }
      }
    }
  }
  else { // 정면이 안 막혔을 때
    if (distance_L <= 30 && distance_R <= 30) { //좌우가 다 막히면, U턴
      drive_line_body_frame(0, 0, 20, 1000);
    }
    else if (distance_L <= 30) { //좌측 막히면 우측으로 회전
      drive_line_body_frame(0, 0, 5, 500);
    }
    else if (distance_R <= 30) { //우측 막히면 좌측으로 회전
      drive_line_body_frame(0, 0, -5, 500);
    }
    if (distance_Side_L <= 20) { // 앞이 막혀서 회전 한 후에 옆에 장애물이 가까이 있을 경우 거리를 두도록 한다.
      drive_line_body_frame( 0, 200, 0, 200);
      if ( distance_Side_R <= 20) { // 옆으로 움직였는데 반대쪽 장애물에 가까워진 경우 반대로 다시 살짝 움직여준다.
        drive_line_body_frame( 0, -100, 0, 200);
      }
    }
    else if (distance_Side_R <= 20) {
      drive_line_body_frame( 0, -200, 0, 200);
      if ( distance_Side_L <= 20) {
        drive_line_body_frame( 0, 100, 0, 200);
      }
    }
  }
  drive_line_body_frame(-150, 0, 0, 0);  // 정상주행 앞으로
}

//////////////////////////////////////////////////////////////////////////////
/////////////  함수 정의부/////////////////////////////////////////

////// 초음파센서 /////////

void SetupUltraPin() {
  pinMode(TRIG_Front, OUTPUT);
  pinMode(ECHO_Front, INPUT);

  pinMode(TRIG_Right, OUTPUT);
  pinMode(ECHO_Right, INPUT);

  pinMode(TRIG_Left, OUTPUT);
  pinMode(ECHO_Left, INPUT);

  pinMode(TRIG_SIDE_L, OUTPUT);
  pinMode(ECHO_SIDE_L, INPUT);

  pinMode(TRIG_SIDE_R, OUTPUT);
  pinMode(ECHO_SIDE_R, INPUT);
}

long Read_distance() {
  // 이전 측정 값에 비중을 두고, 현재 측정된 값엔 비중을 낮춤.
  distance_F = UltraSonic(TRIG_Front, ECHO_Front);
  distance_F = round((alpha * distance_F) + ( beta * pre_distance_F));
  if (distance_F <= DIST_MIN) distance_F = DIST_MAX;
  pre_distance_F = distance_F;

  distance_L = UltraSonic(TRIG_Left, ECHO_Left);
  distance_L = round((alpha * distance_L ) + (beta * pre_distance_L));
  if (distance_L <= DIST_MIN) distance_L = DIST_MAX;
  pre_distance_L = distance_L;

  distance_R = UltraSonic(TRIG_Right, ECHO_Right);
  distance_R = round((alpha * distance_R ) + (beta * pre_distance_R));
  if (distance_R <= DIST_MIN) distance_R = DIST_MAX;
  pre_distance_R = distance_R;

  distance_Side_L = UltraSonic(TRIG_SIDE_L, ECHO_SIDE_L);
  distance_Side_L = round((alpha * distance_Side_L) + (beta * pre_distance_Side_L));
  if (distance_Side_L <= DIST_MIN) distance_Side_L = DIST_MAX;
  pre_distance_Side_L = distance_Side_L;

  distance_Side_R = UltraSonic(TRIG_SIDE_R, ECHO_SIDE_R);
  distance_Side_R = round((alpha * distance_Side_R) + (beta * pre_distance_Side_R));
  if (distance_Side_R <= DIST_MIN) distance_Side_R = DIST_MAX;
  pre_distance_Side_R = distance_Side_R;
}

unsigned long UltraSonic(char TRIG, char ECHO) {

  //trigger 발사
  unsigned long distance;

  digitalWrite(TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG, LOW);

  //echo 핀 입력으로 부터 거리를 cm 단위로 계산
  distance = pulseIn(ECHO, HIGH, DIST_S) / 58.2; //최대 측정 사용시 pulseIn(ECHO,HIGH,DIST_S), pulseIn 반환: unsigned long

  if (distance <= DIST_MIN) distance = DIST_MAX;

  return distance;
}

void ShowDistance() {
  Serial.print("distance_F:");
  Serial.println(distance_F);

  Serial.print("distance_L:");
  Serial.println(distance_L);

  Serial.print("distance_R:");
  Serial.println(distance_R);

  Serial.print("distance_Side_L:");
  Serial.println(distance_Side_L);

  Serial.print("distance_Side_R:");
  Serial.println(distance_Side_R);
}


//// Motor Pin Setup ///////////////
void SetupMotorPin() {
  pinMode(BACK_PWM, OUTPUT);
  pinMode(LEFT_PWM, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT);

  pinMode(BACK_DIR, OUTPUT);
  pinMode(LEFT_DIR, OUTPUT);
  pinMode(RIGHT_DIR, OUTPUT);
}

////////// Encoder Interrupt //////
void setInterruptpin() {
  //motor 1 interrupt
  pinMode(BACK_ENCODER_A, INPUT_PULLUP);
  attachInterrupt(0, BackEncoderA, CHANGE);

  pinMode(BACK_ENCODER_B, INPUT_PULLUP);
  attachInterrupt(1, BackEncoderB, CHANGE);

  //motor 2 interrupt
  pinMode(LEFT_ENCODER_A, INPUT_PULLUP);
  attachInterrupt(2, LeftEncoderA, CHANGE);

  pinMode(LEFT_ENCODER_B, INPUT_PULLUP);
  attachInterrupt(3, LeftEncoderB, CHANGE);

  //motor3 interrupt
  pinMode(RIGTH_ENCODER_A, INPUT_PULLUP);
  attachInterrupt(4, RightEncoderA, CHANGE);

  pinMode(RIGTH_ENCODER_B, INPUT_PULLUP);
  attachInterrupt(5, RightEncoderB, CHANGE);
}

// 엔코더 값 읽는 인터럽트 함수
void BackEncoderA() {
  BackencoderPos += (digitalRead(BACK_ENCODER_A) == digitalRead(BACK_ENCODER_B)) ? 1 : -1;
}
void BackEncoderB() {
  BackencoderPos += (digitalRead(BACK_ENCODER_A) == digitalRead(BACK_ENCODER_B)) ? -1 : 1;
}

void LeftEncoderA() {
  LeftencoderPos += (digitalRead(LEFT_ENCODER_A) == digitalRead(LEFT_ENCODER_B)) ? 1 : -1;
}
void LeftEncoderB() {
  LeftencoderPos += (digitalRead(LEFT_ENCODER_A) == digitalRead(LEFT_ENCODER_B)) ? -1 : 1;
}

void RightEncoderA() {
  RightencoderPos += (digitalRead(RIGTH_ENCODER_A) == digitalRead(RIGTH_ENCODER_B)) ? 1 : -1;
}
void RightEncoderB() {
  RightencoderPos += (digitalRead(RIGTH_ENCODER_A) == digitalRead(RIGTH_ENCODER_B)) ? -1 : 1;
}

void ShowEncoderAll() {
  Serial.print("BackencoderPos : ");
  Serial.println(BackencoderPos);
  Serial.print("LeftencoderPos : ");
  Serial.println(LeftencoderPos);
  Serial.print("RightencoderPos : ");
  Serial.println(RightencoderPos);
}

//// Motor Speed 계산부
void generate_matrix_wheel_body(float matrix_w_b[][3], float matrix_b_w[][3]) {
  float angle_between_wheels[3];
  angle_between_wheels[0] = 180 * M_PI / 180; //rad
  angle_between_wheels[1] = (-60) * M_PI / 180; //rad
  angle_between_wheels[2] = 60 * M_PI / 180; //rad

  for (i = 0; i < 3; i++)
  {
    matrix_b_w[i][0] = -BODY_RADIUS;
    matrix_b_w[i][1] = sin(angle_between_wheels[i]);
    matrix_b_w[i][2] = -cos(angle_between_wheels[i]);
  }
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      matrix_w_b[i][j] = matrix_b_w[i][j]; //create a copy
  Matrix.Invert((float*)matrix_w_b, NR_OMNIWHEELS); // this thing stores the inverted matrix in itself

}

bool setDirection(float value) { //모터 방향
  return value > 0;
}

///////  바퀴속도 조정  //////////////
void set_speed(float desired_speed_body_frame[]) {

  back_wheel_speed = right_wheel_speed = left_wheel_speed = 0;

  for (int i = 0; i <= 2; i++)
  {
    back_wheel_speed = back_wheel_speed + matrix_b_w[0][i] * desired_speed_body_frame[i];
    left_wheel_speed = left_wheel_speed + matrix_b_w[2][i] * desired_speed_body_frame[i];
    right_wheel_speed = right_wheel_speed + matrix_b_w[1][i] * desired_speed_body_frame[i];
  }
  // 휠의 회전 방향
  backdir = setDirection(back_wheel_speed);
  leftdir = setDirection(left_wheel_speed);
  rightdir = setDirection(right_wheel_speed);

  // 휠 속도 값
  back_wheel_speed = round(((abs(back_wheel_speed) * 6 * 10.0) / WHEEL_CIRC) * REDUCTION_RATIO_NAMIKI_MOTOR);
  left_wheel_speed = round(((abs(left_wheel_speed) * 6 * 10.0) / WHEEL_CIRC) * REDUCTION_RATIO_NAMIKI_MOTOR);
  right_wheel_speed = round(((abs(right_wheel_speed) * 6 * 10.0) / WHEEL_CIRC) * REDUCTION_RATIO_NAMIKI_MOTOR);

  // 최대 RPM과 비교하여 휠속도 제어
  if (back_wheel_speed >= MAX_SPEEDRPM) back_wheel_speed = MAX_SPEEDRPM;
  if (left_wheel_speed >= MAX_SPEEDRPM) left_wheel_speed = MAX_SPEEDRPM;
  if (right_wheel_speed >= MAX_SPEEDRPM) right_wheel_speed = MAX_SPEEDRPM;

  // map함수를 이용하여 0~MAX_SPEEDRPM을 0~MAX_PWM으로 변환
  back_wheel_speed = map(back_wheel_speed, 0, MAX_SPEEDRPM, 0, MAX_PWM);
  left_wheel_speed = map(left_wheel_speed, 0, MAX_SPEEDRPM, 0, MAX_PWM);
  right_wheel_speed = map(right_wheel_speed, 0, MAX_SPEEDRPM, 0, MAX_PWM);
}

void ShowMotorSPEED() {
  Serial.print("Back Speed: ");
  Serial.println(back_wheel_speed);
  Serial.print("Left Speed: ");
  Serial.println(left_wheel_speed);
  Serial.print("Right Speed: ");
  Serial.println(right_wheel_speed);
}
void ShowMotorDIR() {
  Serial.print("Back Dir: ");
  Serial.println(backdir);
  Serial.print("Left Dir: ");
  Serial.println(leftdir);
  Serial.print("Right Speed: ");
  Serial.println(rightdir);
}

///// Motor 구동 함수 ////////
void drive_line_body_frame(float velocity_x, float velocity_y, float velocity_z, float duration) { // duration: ms
  float desired_speed_body_frame[3] = {0};
  desired_speed_body_frame[0] = velocity_z; // rotations rad/sec
  desired_speed_body_frame[1] = velocity_x; // x directions, mm/sec
  desired_speed_body_frame[2] = velocity_y; // y direction, mm/sec

  set_speed(desired_speed_body_frame);

  BackrunPWM(back_wheel_speed, backdir);
  LeftrunPWM(left_wheel_speed, leftdir);
  RightrunPWM(right_wheel_speed, rightdir);

  if ( duration > 0) {
    delay(duration);
  }
}

void BackrunPWM(unsigned int PWM, bool dir) {
  digitalWrite(BACK_DIR, dir);
  analogWrite(BACK_PWM, PWM);
}
void LeftrunPWM(unsigned int PWM, bool dir) {
  digitalWrite(LEFT_DIR, dir);
  analogWrite(LEFT_PWM, PWM);
}
void RightrunPWM(unsigned int PWM, bool dir) {
  digitalWrite(RIGHT_DIR, dir);
  analogWrite(RIGHT_PWM, PWM);
}

void StopBack() {
  digitalWrite(BACK_DIR, 0);
  analogWrite(BACK_PWM, LOW); // dir이 high이면 255-vel, low이면 vel
}

void StopLeft() {
  digitalWrite(LEFT_DIR, 0);
  analogWrite(LEFT_PWM, LOW); // dir이 high이면 255-vel, low이면 vel
}
void StopRight() {
  digitalWrite(RIGHT_DIR, 0);
  analogWrite(RIGHT_PWM, LOW); // dir이 high이면 255-vel, low이면 vel
}

void StopAll() {
  StopBack();
  StopLeft();
  StopRight();

}
