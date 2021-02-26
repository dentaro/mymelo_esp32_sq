// Arduino  私だけの電子オルゴール 減衰矩形波/減衰正弦波
// 2020.4.21 koyama@hirosaki-u.ac.jp
// 2021.2.26 esp32移植byでんたろう
#include "mymelo.h"

// SQURE_WAVE ---------------------------
#define  V1  0xffff
#define V2  0x7fff
#define V3  0x7fff
uint16_t v1,v2,v3, Va,Vb;             // 振幅とその合算値
uint16_t c1,c2,c3, C1,C2,C3;
uint8_t note1,note2,note3;            // 楽譜データDO|L4
// SQURE_WAVE ---------------------------

#define extSW 34
#define Bzz1  25
#define Bzz2  26

#define CNT 8000000/256*0.5/8*120/TEMPO // テンポ120の四分音符(長さ8)を0.5秒とする
uint16_t cntr=CNT;                    // 32us*(1953*120/Tempo)=62.5ms
uint16_t p1=0, p2=0, p3=0;            // 楽譜part1～part3のポインタ(初期値は先頭)
uint8_t len1=8, len2=8, len3=8;       // 初期ディレイ(62.5ms*8=.5s)
uint8_t decayTimer=0;                 // 32μs*16=480μsごとに減衰させる
uint8_t icntr;                        // チャタリング防止カウンタ(32*255=8.1ms)
uint8_t nPlay=2;                      // 演奏回数(初回は2回演奏)

hw_timer_t * timerA = NULL;//スピーカー用
boolean timer_flag = false;
float vol = 0.0;
int i = 0.0;

void IRAM_ATTR onTimerA() {
  timer_flag = true;
}

void setup() {
  //Serial.begin(115200);
  
  timerA = timerBegin(0, 80, true);//カウント時間は1マイクロ秒
  timerAttachInterrupt(timerA, &onTimerA, true);
  timerAlarmWrite(timerA, 32, true);//8usまで
  timerAlarmEnable(timerA);
  delay(1000);
}
 
void loop() {
  if(timer_flag){
    if(nPlay>0){
    if(--cntr==0){                    // 32us*(1953*120/TEMPO)=62.5ms(32分音符長)
      cntr=CNT;
      if(--len1==0){
        note1=pgm_read_byte(&part1[p1++]);  // part1の次のデータ
        if(note1==0){                 // 0なら今回の演奏終了
          nPlay--;                    // 演奏回数
          p1=p2=p3=0;
          len1=31;len2=len3=32;       // ポーズ(62.5ms*32=2s)
          note1=note2=note3=0;
        }else{
          len1=len[note1>>5];         // 音符は上位3ビット
          c1=0;C1=(C[note1&=0x1f]+64)>>7; // 音は下位5ビット
          v1=V1;                      // 初めは最大振幅
          decayTimer=0;
        }
      }
      if(--len2==0){
        note2=pgm_read_byte(&part2[p2++]);  // part2の次のデータ
        if(note2==0)  p2--;           // 0ならPart2の演奏終了
        else{
          len2=len[note2>>5];         // 音符は上位3ビット
          c2=0;C2=(C[note2&=0x1f]+64)>>7; // 音は下位5ビット
          v2=V2;                      // 初めは最大振幅
        }
      }
      if(--len3==0){
        note3=pgm_read_byte(&part3[p3++]);  // part3の次のデータ
        if(note3==0)  p3--;           // 0ならPart3の演奏終了
        else{
          len3=len[note3>>5];         // 音符は上位3ビット
          c3=0;C3=(C[note3&=0x1f]+64)>>7; // 音は下位5ビット
          v3=V3;                      // 初めは最大振幅
        }
      }
    }
    Vb=Va=0;
    decayTimer++;
    if(note1>1){                      // part1が休符でなければ
      if(++c1==C1)  c1=0;             // 周期は32us*C1
      if(c1<(C1>>1))  Va=v1;          // 周期の前半は振幅Va=v1
      else if(!(decayTimer&0x0f)) v1-=(v1>>8);  // 32us*16=480us毎にv1の振幅減衰
    }
    if(note2>1){                      // part2が休符でなければ
      if(++c2==C2)  c2=0;             // 周期は32us*C2
      if(c2<(C2>>1))  Vb=v2;          // 周期の前半は振幅Vb=v2
      else if(!(decayTimer&0x0f)) v2-=(v2>>8);  // 32us*16=480us毎にv2の振幅減衰
    }
    if(note3>1){                      // part3が休符でなければ
      if(++c3==C3)  c3=0;             // 周期は32us*C3
      if(c3<(C3>>1))  Vb+=v3;         // 周期の前半は振幅Vb+=v3
      else if(!(decayTimer&0x0f)) v3-=(v3>>8);  // 32us*16=480us毎にv3の振幅減衰
    }
      //dacWrite(25,(Va+128)>>8);//part1
      //dacWrite(25,(Vb+128)>>8);//part2
      dacWrite(25,(Va+Vb+256)>>9);//和音
  }
  timer_flag = false;
  }//timer
}
