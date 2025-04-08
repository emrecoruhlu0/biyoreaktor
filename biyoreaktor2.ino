#include <DHT11.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// I2C LCD ayarları
LiquidCrystal_I2C lcd(0x27, 16, 2);  // 0x27 I2C adresi, 16x2 LCD ekran

int simdiki_zaman = 0, eski_zaman = 0;
int sicaklik, nem, isik_seviyesi;
float su_sicaklik;

#define dht_pin 42
#define isik_seviyesi_pin A3
#define su_sicaklik_pin 44
#define gaz_sensoru2_pin A8
#define gaz_sensoru1_pin A9
#define su_seviyesi_pin A10
#define isitici_pin 45  // Röle bağlı olan pin

#define motor1_anahtar_pin 28
#define motor1_relay_pin 31

#define motor2_anahtar_pin 29
#define motor2_relay_pin 32

#define TFT_CS 53
#define TFT_RST 50
#define TFT_DC  48
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

bool motor1_durum = false;
bool motor2_durum = false;
bool button1State = false;
bool lastButton1State = false;
bool button2State = false;
bool lastButton2State = false;

DHT11 dht11(dht_pin);
OneWire oneWire(su_sicaklik_pin);
DallasTemperature DS18B20(&oneWire);

float degerler[7];

// Sensör veri aralıkları
struct SensorAraligi {
  int min;
  int max;
};

SensorAraligi araliklar[7] = {
  {30, 33},  // Sıcaklık aralığı
  {55, 60},  // Nem aralığı
  {300, 700},  // Su seviyesi aralığı
  {200, 800},  // Işık seviyesi aralığı
  {30, 33},  // Su sıcaklığı aralığı
  {100, 400},  // Metan aralığı
  {200, 500}   // Alkol aralığı
};

// İstenen sıcaklık aralığı
const int hedef_sicaklik_min = 30;
const int hedef_sicaklik_max = 33;

// LCD özel karakterler
byte doluKare[8] = {
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111,
  0b11111
};

byte bosKare[8] = {
  0b11111,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b10001,
  0b11111
};

// Dönen karakterler
char spinner[3] = {'/', '|', '-'};
int spinnerIndex = 0;

void setup() {
  Serial.begin(9600);           // Arduino Seri Monitör
  Serial1.begin(115200);        // ESP32 ile iletişim için Serial1
  tft.initR(INITR_144GREENTAB);

  // LCD başlangıç ayarları
  lcd.init();
  lcd.backlight();
  lcd.createChar(0, doluKare);
  lcd.createChar(1, bosKare);

  pinMode(motor1_anahtar_pin, INPUT);
  pinMode(motor1_relay_pin, OUTPUT);
  pinMode(motor2_anahtar_pin, INPUT);
  pinMode(motor2_relay_pin, OUTPUT);
  pinMode(isitici_pin, OUTPUT);  // Isıtıcı pinini çıkış olarak ayarlama

  DS18B20.begin();

  pinMode(su_seviyesi_pin, INPUT);
  pinMode(isik_seviyesi_pin, INPUT);
  pinMode(gaz_sensoru1_pin, INPUT);  // Metan sensörü
  pinMode(gaz_sensoru2_pin, INPUT);  // Alkol sensörü

  // LCD ilk satıra sensör kısaltmalarını yazdırma
  lcd.setCursor(0, 0);
  lcd.print("S:N:L:I:Ss:M:A");
}

void loop() {
  // Buton durumları her döngüde kontrol ediliyor
  
  simdiki_zaman = millis();

  if (simdiki_zaman - eski_zaman >= 1000) {
    DS18B20.requestTemperatures();
    dht11.readTemperatureHumidity(sicaklik, nem); 
    degerler[0] = sicaklik;
    degerler[1] = nem;
    degerler[2] = analogRead(su_seviyesi_pin);
    degerler[3] = analogRead(isik_seviyesi_pin);
    degerler[4] = DS18B20.getTempCByIndex(0);
    degerler[5] = analogRead(gaz_sensoru1_pin);  // Metan sensörü
    degerler[6] = analogRead(gaz_sensoru2_pin);  // Alkol sensörü

    tft.setCursor(0, 5);
    tft.fillScreen(ST77XX_BLACK);

    oled_yazdir(ST77XX_RED, 1, "sicaklik: ", degerler[0], " C"); // sıcaklık değer yazdırma
    oled_yazdir(ST77XX_BLUE, 2, "nem: ", degerler[1], " %"); // nem değer yazdırma
    oled_yazdir(ST77XX_GREEN, 1, "su seviyesi: ", degerler[2], " %"); // su seviyesi değer yazdırma
    oled_yazdir(ST77XX_WHITE, 1, "isik: ", degerler[3], " %"); // ışık seviyesi değer yazdırma
    oled_yazdir(ST77XX_YELLOW, 1, "su sicaklik: ", degerler[4], " C"); // su sıcaklığı yazdırma
    oled_yazdir(ST77XX_RED, 1, "metan: ", degerler[5], " ppm"); // metan değeri yazdırma
    oled_yazdir(ST77XX_GREEN, 1, "alkol: ", degerler[6], " ppm"); // alkol değeri yazdırma
    tft.println("\n");  // satır sonu

    Serial.println("\n\nSensör değerleri:");
    Serial.print("sicaklik degeri: ");
    Serial.println(degerler[0]);
    Serial.print("nem degeri: ");
    Serial.println(degerler[1]);
    Serial.print("su seviyesi degeri: ");
    Serial.println(degerler[2]);
    Serial.print("isik degeri: ");
    Serial.println(degerler[3]);
    Serial.print("su sicaklik degeri: ");
    Serial.println(degerler[4]);
    Serial.print("metan degeri: ");
    Serial.println(degerler[5]);
    Serial.print("alkol degeri: ");
    Serial.println(degerler[6]);

    // ESP32'ye veri gönderme
    for (int i = 0; i < 7; i++) {
      Serial1.print(degerler[i]);
      if (i < 6) {
        Serial1.print(","); // Son veriden sonra virgül eklememek için
      }
    }
    Serial1.println(); // Veri gönderimini yeni satırla sonlandır

    // LCD ikinci satıra verilerin aralıklarda olup olmadığını gösteren kareler
    lcd.setCursor(0, 1);
    for (int i = 0; i < 7; i++) {
      if (degerler[i] >= araliklar[i].min && degerler[i] <= araliklar[i].max) {
        lcd.write(byte(0));  // içi dolu kare
      } else {
        lcd.write(byte(1));  // içi boş kare
      }
      if(i == 4){
        lcd.print(" ");
      }
      lcd.print(" ");  // İki kare arasında ayraç
    }

    // Isıtıcı kontrolü
    if (degerler[0] < hedef_sicaklik_min) {
      digitalWrite(isitici_pin, LOW);  // Isıtıcıyı aç
      Serial.println("Isıtıcı Açıldı.");
    } else if (degerler[0] > hedef_sicaklik_max) {
      digitalWrite(isitici_pin, HIGH);   // Isıtıcıyı kapat
      Serial.println("Isıtıcı Kapandı.");
    }
    button1State = digitalRead(motor1_anahtar_pin);
  if (button1State && !lastButton1State) {
    motor1_durum = !motor1_durum;
    if (motor1_durum) {
      digitalWrite(motor1_relay_pin, HIGH);
      Serial.println("Motor 1 Açıldı.");
    } else {
      digitalWrite(motor1_relay_pin, LOW);
      Serial.println("Motor 1 Kapandı.");
    }
  }

    lastButton1State = button1State;

    button2State = digitalRead(motor2_anahtar_pin);
    if (button2State && !lastButton2State) {
      motor2_durum = !motor2_durum;
      if (motor2_durum) {
        digitalWrite(motor2_relay_pin, HIGH);
        Serial.println("Motor 2 Açıldı.");
      } else {
        digitalWrite(motor2_relay_pin, LOW);
        Serial.println("Motor 2 Kapandı.");
      }
    }
    lastButton2State = button2State;
  
  
    // LCD sağ alt köşeye dönen karakter ekleme
    lcd.setCursor(15, 1);
    lcd.print(spinner[spinnerIndex]);
    spinnerIndex = (spinnerIndex + 1) % 3;

    eski_zaman = millis();
  }
}

void oled_yazdir(uint16_t renk, int boyut, char *sensor, int deger, char *birim) {
  tft.setTextColor(renk);
  tft.setTextSize(boyut);
  tft.print(sensor);
  tft.print(deger);
  tft.print(birim);
  tft.println("\n");
}
