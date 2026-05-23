#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>
#include <cstdio>

// ===== OLED =====
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ===== Stałe / kalibracje =====
unsigned long time_start, time_koniec;
float C_offset = 755.5f * 1e-12f;
float L_offset = 100e-6;

float v_ref = 3.3f;
float resoulution = 4095.0f;

// R 
float R_wzorcowe_1  = 992.4f;     // ~1k
float R_wzorcowe_2  = 24580.0f;   // ~25k

// C
float R_C_10M = 10.0e6f;   // <-- najlepiej podmień na zmierzone
float R_C_10K = 10.0e3f;   // <-- najlepiej podmień na zmierzone

// L
float C_wzorcowe = 0.000000042f; // 42 nF

// ===== Piny =====
// wyjścia
int R_pomiar_1k   = 11;
int R_pomiar_25k  = 12;

int C_pomiar_10M  = 13;  // CHARGE
int C_pomiar_10k  = 15;  // FAST DISCHARGE

int L_pomiar      = 14;
int Led_power_on  = 1;

int wynik_pomiaru_R = A1;
int wynik_pomiaru_C = A0;
int wynik_pomiaru_L = 2;  

int R_button      = 17; 
int C_button      = 16;
int L_button      = 19;
int start         = 18;

// ========================= OLED helpers =========================
void Clearline(uint8_t linia, uint8_t size) {
  uint8_t y = linia * 8;
  uint8_t h = 8 * size;
  if (y >= SCREEN_HEIGHT) return;
  display.fillRect(0, y, SCREEN_WIDTH, h, SSD1306_BLACK);
}

void slowodruk(const char* tekst, uint8_t linia, int Size) {
  Clearline(linia, Size);
  display.setTextSize(Size);
  display.setTextColor(SSD1306_WHITE);
  uint8_t y = linia * 8;
  if (y >= SCREEN_HEIGHT) return;
  display.setCursor(0, y);
  display.print(tekst);
  display.display();
}

// ========================= Format helpers =========================
const char* drukRes(float R){
  static char buf[24];
  if (isnan(R) || R <= 0.0f) { snprintf(buf, sizeof(buf), "R=ERR"); return buf; }

  float v = R;
  const char* u = "ohm";
  if (v >= 1e9f)      { v /= 1e9f; u = "Gohm"; }
  else if (v >= 1e6f) { v /= 1e6f; u = "Mohm"; }
  else if (v >= 1e3f) { v /= 1e3f; u = "kohm"; }

  snprintf(buf, sizeof(buf), "%.2f%s", v, u);
  return buf;
}

const char* drukCap(float F){
  static char buf[24];
  if (isnan(F) || F <= 0.0f) { snprintf(buf, sizeof(buf), "C=ERR"); return buf; }

  float v = F; const char* u = "F";
  if (v >= 1e-3f)      { v *= 1e3f;  u = "mF"; }
  else if (v >= 1e-6f) { v *= 1e6f;  u = "uF"; }
  else if (v >= 1e-9f) { v *= 1e9f;  u = "nF"; }
  else                 { v *= 1e12f; u = "pF"; }

  snprintf(buf, sizeof(buf), "%.2f %s", v, u);
  return buf;
}

const char* drukFreq(float f){
  static char buf[24];
  if (!isfinite(f) || f <= 0.0f) { snprintf(buf, sizeof(buf), "f=ERR"); return buf; }

  float v = f; const char* u = "Hz";
  if (v >= 1e6f)      { v /= 1e6f; u = "MHz"; }
  else if (v >= 1e3f) { v /= 1e3f; u = "kHz"; }

  snprintf(buf, sizeof(buf), "f=%.2f %s", v, u);
  return buf;
}

const char* drukInd(float L){
  static char buf[24];
  if (!isfinite(L) || L <= 0.0f) { snprintf(buf, sizeof(buf), "L=ERR"); return buf; }

  float v = L; const char* u = "H";
  if (v < 1e-3f)      { v *= 1e6f; u = "uH"; }
  else if (v < 1.0f)  { v *= 1e3f; u = "mH"; }

  snprintf(buf, sizeof(buf), "%.2f %s", v, u);
  return buf;
}

// ========================= Pomiar R =========================
static float Pomiar_R(){
  pinMode(R_pomiar_1k, OUTPUT);  digitalWrite(R_pomiar_1k, HIGH);
  pinMode(R_pomiar_25k, INPUT);
  delay(200);
  float value_1 = analogRead(wynik_pomiaru_R);

  pinMode(R_pomiar_25k, OUTPUT); digitalWrite(R_pomiar_25k, HIGH);
  pinMode(R_pomiar_1k, INPUT);
  delay(200);
  float value_2 = analogRead(wynik_pomiaru_R);

  digitalWrite(R_pomiar_25k, LOW);
  delay(50);

  float srodek = resoulution / 2.0f;
  float A_1 = value_1 - srodek;
  float A_2 = value_2 - srodek;

  float R_ref, v_adc;
  if (fabsf(A_2) > fabsf(A_1)) { R_ref = R_wzorcowe_1; v_adc = value_1; }
  else                         { R_ref = R_wzorcowe_2; v_adc = value_2; }

  float v_meas = v_adc * v_ref / resoulution;
  float v_refdrop = v_ref - v_meas;
  float I = v_refdrop / R_ref;
  if (I <= 0.0f) return NAN;

  return v_meas / I;
}

// ========================= Pomiar C  =========================
static inline void C_chargeOn()        { digitalWrite(C_pomiar_10M, HIGH); } // Q5 ON
static inline void C_chargeOff()       { digitalWrite(C_pomiar_10M, LOW);  } // Q5 OFF
static inline void C_fastDisOn()       { digitalWrite(C_pomiar_10k, HIGH); } // Q4 ON -> 10k do GND
static inline void C_fastDisOff()      { digitalWrite(C_pomiar_10k, LOW);  } // Q4 OFF -> zostaje tylko 10M

static void C_fullDischarge(uint16_t ms=200) {
  // szybko rozładuj żeby start zawsze był powtarzalny
  C_chargeOff();
  C_fastDisOn();
  delay(ms);
  C_fastDisOff();
}

// ładowanie do ~3.3V (ale z limitem czasu, żeby nie wisieć wiecznie)
static float C_chargeAndReadV0() {
  C_fastDisOff();   // podczas ładowania fast-discharge wyłączony
  C_chargeOn();
  uint32_t t0 = millis();

  float v = 0;
  while (true) {
    (void)analogRead(wynik_pomiaru_C);
    v = (float)analogRead(wynik_pomiaru_C);

    if (v > 0.98f * resoulution) break;       
    if ((millis() - t0) > 400) break;        
  }

  C_chargeOff();
  delayMicroseconds(200);

  (void)analogRead(wynik_pomiaru_C);
  v = (float)analogRead(wynik_pomiaru_C);
  return v;
}

static float C_measureDischarge_us(bool fast, uint32_t timeout_us) {
  float v0 = C_chargeAndReadV0();
  if (v0 < 0.2f * resoulution) return NAN;

  float thr = v0 * 0.367879f;

  // start rozładowania
  if (fast) C_fastDisOn();
  else      C_fastDisOff(); // zostaje 10M

  uint32_t t_start = micros();
  while (true) {
    float v = (float)analogRead(wynik_pomiaru_C);
    if (v <= thr) break;
    if ((micros() - t_start) > timeout_us) {
      if (fast) C_fastDisOff();
      return NAN;
    }
  }
  uint32_t t_end = micros();

  if (fast) C_fastDisOff();
  return (float)(t_end - t_start);
}

static float C_req(bool fast) {
  if (!fast) return R_C_10M;
  float denom = (1.0f / R_C_10K) + (1.0f / R_C_10M);
  if (denom <= 0.0f) return NAN;
  return 1.0f / denom;
}

static float Pomiar_C() {
  // autorange:
  // 1) próbuj slow (10M) z krótkim timeout
  // 2) jak za długo -> fast (10k) z długim timeout (pod duże elektrolity)
  // + uśrednianie 3 pomiarów w wybranym trybie

  // najpierw wymuś znany start
  C_fullDischarge(120);

  bool fast = false;
  float t_us = C_measureDischarge_us(false, 1200000UL); // 1.2s

  if (!isfinite(t_us)) {
    fast = true;
    C_fullDischarge(120);
    t_us = C_measureDischarge_us(true, 12000000UL);     // 12s (ok. do ~1mF przy 10k)
  }
  if (!isfinite(t_us)) return NAN;

  // uśrednij 3 pomiary
  float sum = t_us;
  int cnt = 1;
  for (int i = 0; i < 2; i++) {
    C_fullDischarge(80);
    float ti = C_measureDischarge_us(fast, fast ? 12000000UL : 1200000UL);
    if (isfinite(ti)) { sum += ti; cnt++; }
  }
  float avg_us = sum / (float)cnt;

  float R = C_req(fast);
  if (!isfinite(R) || R <= 0.0f) return NAN;

  float C = (avg_us * 1e-6f) / R; // t = R*C
  C -= C_offset;
  if (C < 0.0f) C = 0.0f;

  // rozładuj na koniec
  C_fullDischarge(120);
  return C;
}

// ========================= Pomiar L (burst + LM339) =========================
const uint16_t MIN_EDGES    = 5;
const uint32_t STOP_GAP_US  = 250;
const uint32_t MAX_WAIT_US  = 5000;
const uint16_t PULSE_US     = 10; 

volatile uint32_t t_first = 0;
volatile uint32_t t_last  = 0;
volatile uint32_t t_edge  = 0;
volatile uint16_t edges   = 0;

void onEdgeISR() {
  uint32_t t = micros();
  if (edges == 0) t_first = t;
  t_last = t;
  t_edge = t;
  edges++;
}

float measureFreqBurstHz() {
  noInterrupts();
  edges = 0;
  t_first = t_last = t_edge = 0;
  interrupts();

  attachInterrupt(digitalPinToInterrupt(wynik_pomiaru_L), onEdgeISR, FALLING);

  // wzbudzenie
  digitalWrite(L_pomiar, HIGH);
  delayMicroseconds(PULSE_US);
  digitalWrite(L_pomiar, LOW);

  uint32_t t0 = micros();
  while (true) {
    uint32_t now = micros();

    uint16_t e;
    uint32_t lastE;
    noInterrupts();
    e = edges;
    lastE = t_edge;
    interrupts();

    if (e >= MIN_EDGES && (now - lastE) > STOP_GAP_US) break;
    if ((now - t0) > MAX_WAIT_US) break;
  }

  detachInterrupt(digitalPinToInterrupt(wynik_pomiaru_L));

  uint16_t e;
  uint32_t a, b;
  noInterrupts();
  e = edges;
  a = t_first;
  b = t_last;
  interrupts();

  if (e < 2 || b <= a) return NAN;

  float dt_us = (float)(b - a);
  float f = (float)(e - 1) * 1e6f / dt_us;
  return f;
}

float Pomiar_L() {
  // proste uśrednianie 3 burstów (stabilniejsze)
  float fsum = 0; int n = 0;
  for (int i = 0; i < 3; i++) {
    float f = measureFreqBurstHz();
    if (isfinite(f) && f > 0.0f) { fsum += f; n++; }
    delay(30);
  }
  if (n == 0) return NAN;

  float f = fsum / (float)n;
  float w = 6.28318530718f * f;
  return 1.0f / (w * w * C_wzorcowe);
}

// ========================= UI: wybór trybu + START =========================
enum Mode { MODE_NONE, MODE_R, MODE_C, MODE_L };
Mode aktualny = MODE_NONE;

bool lastR = true, lastC = true, lastL = true, lastS = true;
uint32_t dbR=0, dbC=0, dbL=0, dbS=0;

bool pressedEdge(int pin, bool &lastState, uint32_t &lastDbMs) {
  bool now = digitalRead(pin); // pullup: true=puszczony, false=wcisniety
  bool ev = false;
  if (lastState == true && now == false) {
    uint32_t t = millis();
    if (t - lastDbMs > 160) { ev = true; lastDbMs = t; }
  }
  lastState = now;
  return ev;
}

void pokazTryb() {
  Clearline(1, 2);

  if (aktualny == MODE_NONE) {
    slowodruk("Wybierz pomiar:", 0, 1);
    slowodruk("R / C / L  + Start", 1, 1);
    return;
  }

  if (aktualny == MODE_R) slowodruk("Aktualny pomiar: R", 0, 1);
  if (aktualny == MODE_C) slowodruk("Aktualny pomiar: C", 0, 1);
  if (aktualny == MODE_L) slowodruk("Aktualny pomiar: L", 0, 1);

  slowodruk("Wcisnij Start", 1, 1);

  Clearline(2, 2);
  Clearline(3, 2);
}

void setup() {
  Serial.begin(9600);
  analogReadResolution(12);
  delay(200);

  pinMode(R_pomiar_1k, OUTPUT);    digitalWrite(R_pomiar_1k, LOW);
  pinMode(R_pomiar_25k, OUTPUT);   digitalWrite(R_pomiar_25k, LOW);

  pinMode(C_pomiar_10M, OUTPUT);   digitalWrite(C_pomiar_10M, LOW); // CHARGE OFF
  pinMode(C_pomiar_10k, OUTPUT);   digitalWrite(C_pomiar_10k, LOW); // FAST OFF

  pinMode(L_pomiar, OUTPUT);       digitalWrite(L_pomiar, LOW);

  pinMode(wynik_pomiaru_R, INPUT);
  pinMode(wynik_pomiaru_C, INPUT);
  pinMode(wynik_pomiaru_L, INPUT);
  pinMode(R_button, INPUT_PULLUP);
  pinMode(C_button, INPUT_PULLUP);
  pinMode(L_button, INPUT_PULLUP);
  pinMode(start,    INPUT_PULLUP);

  pinMode(Led_power_on, OUTPUT);
  digitalWrite(Led_power_on, HIGH);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    while (1) { delay(100); }
  }
  display.clearDisplay();
  display.display();

  pokazTryb();
}

void loop() {
  if (pressedEdge(R_button, lastR, dbR)) { aktualny = MODE_R; pokazTryb(); }
  if (pressedEdge(C_button, lastC, dbC)) { aktualny = MODE_C; pokazTryb(); }
  if (pressedEdge(L_button, lastL, dbL)) { aktualny = MODE_L; pokazTryb(); }

  if (pressedEdge(start, lastS, dbS)) {
    if (aktualny == MODE_NONE) { pokazTryb(); return; }

    slowodruk("Mierze...", 0, 1);
    Clearline(1, 2);
    Clearline(2, 2);
    Clearline(3, 2);

    if (aktualny == MODE_R) {
      float r = Pomiar_R();
      slowodruk(drukRes(r), 1, 2);
      Serial.print(""); Serial.println(r, 6);
    }
    else if (aktualny == MODE_C) {
      float c = Pomiar_C();
      slowodruk(drukCap(c), 1, 2);
      Serial.print(""); Serial.println(c, 12);
    }
    else if (aktualny == MODE_L) {
      float f = measureFreqBurstHz();
      float L = NAN;
      if (isfinite(f) && f > 0.0f) {
        float w = 6.28318530718f * f;
        L = 1.0f / (w * w * C_wzorcowe);
      }

      slowodruk(drukFreq(f), 0, 1);
      slowodruk(drukInd(L-L_offset),  1, 2);

      Serial.print("edges="); Serial.print((int)edges);
      Serial.print("  f=");   Serial.print(f, 2); Serial.print(" Hz");
      Serial.print("  ");   Serial.print(L, 12); Serial.println(" H");
    }
  }
}
