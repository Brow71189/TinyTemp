#define TACHO_PIN_PWM 8
#define TEMP_PIN 1
#define SENSITIVITY_PIN 2
#define PWM_PIN 6
// in Ohm
#define R_REF_THERMISTOR 10000
#define R_REF_SENSITIVITY 6800
#define R_MAX_SENSITIVITY 1000 // maximum resistivity of potentiometer

#define ADC_MAX_VALUE 1024
#define FAN_PWM_MAX_VALUE 255
#define FAN_PWM_MIN_VALUE 16 //this is the minimum duty cycle for the fan

#define THERMISTOR_B 4050
#define THERMISTOR_R_INF .012522057

// in °C
float T_min = 30.;
float T_max = 120.;

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(PWM_PIN, OUTPUT);
  pinMode(TEMP_PIN, INPUT);
  pinMode(SENSITIVITY_PIN, INPUT);
  pinMode(TACHO_PIN_PWM, OUTPUT);

  /*
   * WGM10, WGM12: Fast PWM, 8-bit TOP=255
   * CS10: No prescaler
   * COM1A1: Pin 6 to LOW on match with OCR1A and to HIGH on TOP
   */
  TCCR1A = _BV(COM1A1) | _BV(WGM10);
  TCCR1B = _BV(CS10) | _BV(WGM12);
  
  //counter0 same settings as counter0
  TCCR0A = _BV(COM0A1) | _BV(WGM01) | _BV(WGM00);
  TCCR0B = _BV(CS00);

  // counter0
  // toggle OC0A on compare match, Fast PWM with TOP set by OCR0A
  //TCCR0A = _BV(COM0A0) | _BV(WGM01) | _BV(WGM00);
  // prescaler 1024
  //TCCR0B = _BV(CS02) | _BV(CS00) | _BV(WGM02) ;
  // TOP for timer0 ~4 kHz divided by this value + 1 is output frequency 
  //OCR0A = 41;

    /*
   * 32 kHz with 8MHz CPU clock
   */
  //compare for timer1
  OCR1A = 200;
  //compare for timer0
  OCR0A = 7;
}

// the loop function runs over and over again forever
void loop() {
  update_t_max();
  delay(16000);
  update_fan_speed();
  //set_pwm_from_potentiometer();
}

// the variable resistor must be connected to Ground and the known one to Vcc.
float read_resistivity(byte pin, float R_ref) {
  float raw = 0.;
  for (int i=0; i <= 100; i++) {
    raw += (float)analogRead(pin);
  }
  raw /= 100.;
  // make sure we return something sensible if R is inf (raw == ADC_MAX_VALUE)
  if (raw >= ADC_MAX_VALUE-1) {
    return 1e9;
  }
  return R_ref*(float)raw/((float)ADC_MAX_VALUE - (float)raw);
}

float calculate_temperature(float R) {
  return (float)THERMISTOR_B/log(R/THERMISTOR_R_INF) - 273.15;
}

int calculate_pwm_value(float T) {
  // translates T to a PWM duty cycle if T between T_min and T_max with linear scaling
  if (T < T_min){
    return FAN_PWM_MIN_VALUE;
  } else if (T > T_max) {
    return FAN_PWM_MAX_VALUE;
  } else {
    return (int)(((float)(FAN_PWM_MAX_VALUE - FAN_PWM_MIN_VALUE) / (T_max - T_min)) *
           (T - T_min) + FAN_PWM_MIN_VALUE);
  }
}

void update_fan_speed() {
  float resistivity = read_resistivity(TEMP_PIN, (float)R_REF_THERMISTOR);
  float temperature = calculate_temperature(resistivity);
  OCR1A = calculate_pwm_value(temperature);
  //if (OCR1A == 127) {
  //  OCR1A = 255;
  //} else {
  //  OCR1A = 127;
  //}
}

void update_t_max() {
  float resistivity = read_resistivity(SENSITIVITY_PIN, (float)R_REF_SENSITIVITY);
  // adjust T_max in range from 60 to 200 C
  // make sure we are in a sensible state if potentioneter is disconnected
  if (resistivity > 10*R_REF_SENSITIVITY) {
    T_max = 120.;
    return;
  }

  T_max = 140./(float)R_MAX_SENSITIVITY * resistivity + 60.;
}

void set_pwm_from_potentiometer() {
  float resistivity = read_resistivity(SENSITIVITY_PIN, (float)R_REF_SENSITIVITY);
  OCR1A = (int)(((float)(FAN_PWM_MAX_VALUE - FAN_PWM_MIN_VALUE)/(float)R_MAX_SENSITIVITY)*
                  resistivity + (float)FAN_PWM_MIN_VALUE);
}
