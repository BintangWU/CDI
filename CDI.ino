#define Fosc                    16000000UL        //16MHZ
#define Tosc                    1000000UL / Fosc  //Periode T = 1/F in second
#define Prescaler               8
#define COMPARE_VALUE(n)        (Fosc / (Prescaler * (1000000 / n))) - 1

#define MS_IN_MINUTE            60000
#define US_IN_MINUTE            60000000
#define MAX_RPM                 20000

#define SPARK_COMPARE           OCR1A
#define SPARK_COUNTER           TCNT1
#define SPARK_TIMER_ENABLE      TIMSK1 |= (1 << OCIE1A);
#define SPARK_TIMER_DISABLE     TIMSK1 &= ~(1 << OCIE1A);

#define PULSER_PIN              19
#define SPARK_PIN               36
#define IGNITION_ON             digitalWrite(SPARK_PIN, HIGH)
#define IGNITION_OFF            digitalWrite(SPARK_PIN, LOW)

byte crankingAngle; //Sudut standart/idle
volatile unsigned int sparkDelayTime = 500;   // uS
volatile unsigned int sparkOnTime = 2000;     // uS
volatile bool sparkOn;
volatile bool triggerOK = false;

volatile unsigned long pickUpTime;
volatile unsigned long lastPickUpTime;
unsigned long revolutionTime;

uint16_t RPM;
uint16_t timePerDegree = 0;

void setup() {
  Serial.begin(115200);

  TCCR1A = 0;
  TCCR1B = 0;
  SPARK_TIMER_DISABLE;

  detachInterrupt(digitalPinToInterrupt(PULSER_PIN));
  pinMode (SPARK_PIN, OUTPUT);
  pinMode (PULSER_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PULSER_PIN), handlerTrigger, RISING);
}

void loop() {
  RPM = readRPM();
  timePerDegree = revolutionTime / 360;
  //  Serial.print("RPM: ");
  //  Serial.print(RPM);
  //  Serial.print(",");
  //  Serial.print("DegreePerUs: ");
  //  Serial.print(timePerDegree);
  //  Serial.print(",");
  //  Serial.print("Time: ");
  //  Serial.println(timePerDegree * 36);

  if (triggerOK)
    beginIgnition();    
  triggerOK = false;

  noInterrupts();
  sparkDelayTime = timePerDegree * 36;
  sparkOnTime = 2000;
  interrupts();
}

uint16_t readRPM() {
  uint16_t rpm = 0;

  if ((pickUpTime == 0) || (lastPickUpTime == 0))
    rpm = 0;
  else {
    noInterrupts();
    revolutionTime = pickUpTime - lastPickUpTime;
    interrupts();
    revolutionTime = revolutionTime / 2;
    rpm = US_IN_MINUTE / revolutionTime;

    if (rpm > MAX_RPM)
      return MAX_RPM;
    else
      return rpm;
  }
}

void handlerTrigger() {
  lastPickUpTime = pickUpTime;
  pickUpTime = micros();
  triggerOK = true;
}

void beginIgnition() {
  sparkOn = false;
  TCCR1A = 0;
  TCNT1 = 0;
  TCCR1B = bit(WGM12) | bit(CS11);
  SPARK_COMPARE = COMPARE_VALUE(sparkDelayTime);
  SPARK_TIMER_ENABLE;
}

ISR (TIMER1_COMPA_vect) {
  if (sparkOn) {
    digitalWrite(SPARK_PIN, LOW);
    TCCR1B = 0;
    TIMSK1 = 0;
  }
  else {
    digitalWrite(SPARK_PIN, HIGH);
    TCCR1B = 0;
    TCNT1 = 0;
    TCCR1B = bit(WGM12) | bit(CS11);
    SPARK_COMPARE = COMPARE_VALUE(sparkOnTime);
  }
  sparkOn = !sparkOn;
}
