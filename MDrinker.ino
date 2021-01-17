#include <ezButton.h>
#include <Servo.h>
#include <avr/eeprom.h>
#include <Adafruit_NeoPixel.h>

// Статусы 
#define NONE -1
#define READY 0
#define FILL 1
#define FINISH 2
#define MAX_SHOTS 6

// Пины питания
#define POMP_PWR_PIN 5
#define SERVO_PWR_PIN 6
#define GREEN_LED_PIN 15
#define RED_LED_PIN 16
#define BLUE_LED_PIN 17
#define DISK_LED_PIN 18
#define DISK_LED_COUNT 8

ezButton button_00(12);
ezButton button_01(11);
ezButton button_02(10);
ezButton button_03(9);
ezButton button_04(8);
ezButton button_05(7);

ezButton button_service_00(3);
ezButton button_service_01(2);
ezButton button_service_02(14);
Servo servomotor;
bool service_mode = false;
int filled_timeout = 3000;
unsigned long fill_timestamp = 0;
unsigned long filled_timestamp = 0;
unsigned long filledd_timestamp = 0;
//unsigned long test = 0;
int fill_button = 0;

int click_count = 0;
long click_timestamp = 0;
long click_filled = 0;
//int positions[MAX_SHOTS] = {0, 30, 65, 100, 135, 170};
int positions[MAX_SHOTS] = {170, 135, 100, 65, 30, 0};
int queues[MAX_SHOTS] = {NONE, NONE, NONE, NONE, NONE, NONE};
int queues_pos = 0;
int buttons_status[MAX_SHOTS] = {NONE, NONE, NONE, NONE, NONE, NONE};

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(MAX_SHOTS * DISK_LED_COUNT, DISK_LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() { 
  Serial.begin(9600);
  servomotor.attach(4);
  //servomotor.write(15);

  pinMode(POMP_PWR_PIN, OUTPUT);
  pinMode(SERVO_PWR_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  servo_write(10);

  eeprom_write_word(0, filled_timeout);
  if (filled_timeout < 1000 || filled_timeout > 7000) {
    filled_timeout = 3000;
  }

  pixels.begin();
  pixels.show(); // Устанавливаем все светодиоды в состояние "Выключено"

  for (int i = 0; i < MAX_SHOTS * DISK_LED_COUNT; i++) {
    pixels.setPixelColor(i, pixels.Color(0,0,250)); // Назначаем для первого светодиода цвет "Синий"
    pixels.show();
    delay(20);
  }

    for (int i = 0; i < MAX_SHOTS * DISK_LED_COUNT; i++) {
    pixels.setPixelColor(i, pixels.Color(0,0,0)); // Назначаем для первого светодиода цвет "Синий"
    pixels.show();
  }
}


void led_disk(int button_idx, int led_status) {
    for (int i = button_idx * DISK_LED_COUNT; i < button_idx * DISK_LED_COUNT + DISK_LED_COUNT; i++) {
      if (led_status == READY) {
        pixels.setPixelColor(i, pixels.Color(243, 98, 35)); // Назначаем для первого светодиода цвет "Синий"
        pixels.show();
      } else if(led_status == NONE) {
        pixels.setPixelColor(i, pixels.Color(0, 0, 0)); // Назначаем для первого светодиода цвет "Синий"
        pixels.show();
      } else if(led_status == FINISH) {
        pixels.setPixelColor(i, pixels.Color(50, 205, 50)); // Назначаем для первого светодиода цвет "Синий"
        pixels.show();
      }
  }

  if(led_status == FILL && filled_timestamp) {
    // led_disk(button_idx, NONE);

    int tmp = filled_timestamp - millis();
    int led_index = DISK_LED_COUNT - (tmp / (filled_timeout / DISK_LED_COUNT));
    for (int i = button_idx * DISK_LED_COUNT; i < button_idx * DISK_LED_COUNT + led_index; i++) {
      pixels.setPixelColor(i, pixels.Color(50, 205, 50)); // Назначаем для первого светодиода цвет "Синий" 
      pixels.show();
    }
    //delay(50);
  }
}


void servo_write(int angle) { 
  digitalWrite(SERVO_PWR_PIN, HIGH);
  servomotor.write(angle);
  delay(1300);
  digitalWrite(SERVO_PWR_PIN, LOW);
}


void loop() {
  button_00.loop();
  button_01.loop();
  button_02.loop();
  button_03.loop();
  button_04.loop();
  button_05.loop();
  button_service_00.loop();
  button_service_01.loop();
  button_service_02.loop();

  if (!service_mode) {
    for (int i = 0; i < MAX_SHOTS; i++) {
      if (button_00.getState() && button_01.getState() && button_02.getState() && button_03.getState() && button_04.getState() && button_05.getState()) {
        queues[i] = NONE;
        buttons_status[i] = NONE;
        digitalWrite(POMP_PWR_PIN, LOW);
        filled_timestamp = 0;
        fill_button = NONE;
      }
    }
  }

  for (int i = 0; i < MAX_SHOTS; i++) {
    led_disk(i, buttons_status[i]);
  }
  
  fill_loop();

  if (click_timestamp && millis() > click_timestamp) {
    click_timestamp = 0;
    click_count = 0;
  }

  if (click_count >= 3) {
    Serial.println("SERVICE MODE");
    service_mode = true;
    click_timestamp = 0;
    click_count = 0;
    servo_write(positions[0]);
    digitalWrite(BLUE_LED_PIN, HIGH);
    delay(500);
  }

  if (!service_mode) {
    if (button_service_02.isPressed()) {
      digitalWrite(BLUE_LED_PIN, HIGH);
      click_count += 1;
      click_timestamp = millis() + 1000;
    } else {
      digitalWrite(BLUE_LED_PIN, LOW);
    }
  }

  if (service_mode) {
    if (!button_00.getState()) {
      if (!button_service_02.getState()) {
        if (!click_filled) {
          click_filled = millis();
        }
        
        digitalWrite(POMP_PWR_PIN, HIGH);
      } else {
        if (click_filled) {
          Serial.println(millis() - click_filled);
          filled_timeout = millis() - click_filled;
          eeprom_write_word(0, filled_timeout); 
          click_filled = 0;
        }
        digitalWrite(POMP_PWR_PIN, LOW);
      }
    }
 
    if (!button_service_01.getState() && button_00.getState()) {
      service_mode = false;
      digitalWrite(BLUE_LED_PIN, LOW);
    }
  }

  if (!service_mode) {
    if (!button_00.getState()) {
        Serial.println("button_00");
        add_button_queue(5);
    }
    else if (button_00.getState()) {
      remove_button_queue(5);
    }
  
    if (!button_01.getState()) {
        add_button_queue(4);
    }
    else if (button_01.getState()) {
      remove_button_queue(4);
    }
  
    if (!button_02.getState()) {
        add_button_queue(3);
    }
    else if (button_02.getState()) {
      remove_button_queue(3);
    }
  
    if (!button_03.getState()) {
        add_button_queue(2);
    }
    else if (button_03.getState()) {
      remove_button_queue(2);
    }

    if (!button_04.getState()) {
        add_button_queue(1);
    }
    else if (button_04.getState()) {
      remove_button_queue(1);
    }

    if (!button_05.getState()) {
        add_button_queue(0);
    }
    else if (button_05.getState()) {
      remove_button_queue(0);
    }
  }
  delay(50);
}

void add_button_queue(int button_idx) {
  if (check_button_queue(button_idx) != NONE) {
    return;
  }
  
  int idx = get_queue_free();
  queues[idx] = button_idx;
  buttons_status[button_idx] = READY;
}

void remove_button_queue(int button_idx) {
  int idx = check_button_queue(button_idx);
  if (idx == NONE) {
    return;
  }

  if (buttons_status[button_idx] == FILL) {
    digitalWrite(POMP_PWR_PIN, LOW);
    fill_button = NONE;
    fill_timestamp = 0;
    filled_timestamp = 0;
    filledd_timestamp = 0;
  }

  queues[idx] = NONE;
  buttons_status[button_idx] = NONE;

  for (int i = idx; i < MAX_SHOTS - 1; i++) {
    queues[i] = queues[i + 1];
    queues[i + 1] = NONE;
  }
}

int get_queue_free() {
  for (int i = 0; i < MAX_SHOTS; i++) {
    if (queues[i] == NONE) {
      return i;
    }
  }
  return NONE;
}

int check_button_queue(int button_idx) {
  for (int i = 0; i < MAX_SHOTS; i++) {
    if (queues[i] == button_idx) {
      return i;
    }
  }
  return NONE;
}

bool fill_check(int status) {
  for (int i = 0; i < MAX_SHOTS; i++) {
    if (buttons_status[i] == status) {
      return true;
    }
  }
  return false;
}

void fill_loop() {
  for (int i = 0; i < MAX_SHOTS; i++) {
    if (queues[i] != -1 && buttons_status[queues[i]] != FINISH) {
      if (fill_check(FILL)) {
        continue;
      }
      
      int button_idx = queues[i];
      
      buttons_status[button_idx] = FILL;
      fill_timestamp = millis() + 1500;
      fill_button = button_idx;
      break;
    }
  }
  //test

  if (millis() > fill_timestamp && fill_timestamp) {
    servo_write(positions[fill_button]);
    fill_timestamp = 0;
    delay(500);
    digitalWrite(POMP_PWR_PIN, HIGH);
    filled_timestamp = millis() + filled_timeout;
  }

  if (millis() > filled_timestamp && filled_timestamp) {
    digitalWrite(POMP_PWR_PIN, LOW);
    filled_timestamp = 0;
    buttons_status[fill_button] = FINISH;
    fill_button = NONE;
  }
}
