#define BLYNK_TEMPLATE_ID "TMPL3mi1lDTCO"
#define BLYNK_TEMPLATE_NAME "Soil Monitoring"
#define BLYNK_AUTH_TOKEN "YOUR_BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <U8g2lib.h>

// =====================================================
// WiFi
// =====================================================

char ssid[] = "uk";
char pass[] = "12345678";

// =====================================================
// 1.3 inch 128x64 OLED
// SH1106 I2C
// =====================================================

#define OLED_SDA 21
#define OLED_SCL 22

U8G2_SH1106_128X64_NONAME_F_HW_I2C display(
    U8G2_R0,
    U8X8_PIN_NONE
);

// =====================================================
// RS485 Soil Sensor
// =====================================================

#define RXD2 16
#define TXD2 17
#define RE_DE 4

uint8_t requestData[8] =
{
    0x01,
    0x03,
    0x00,
    0x00,
    0x00,
    0x07,
    0x04,
    0x08
};

uint8_t responseData[19];

// =====================================================
// Blynk Timer
// =====================================================

BlynkTimer timer;

// =====================================================
// Sensor Values
// =====================================================

float moisture = 0.0;
float temperature = 0.0;
int ec = 0;
float ph = 0.0;
int nitrogen = 0;
int phosphorus = 0;
int potassium = 0;

// =====================================================
// OLED
// =====================================================

void updateDisplay()
{
    display.clearBuffer();

    display.setFont(u8g2_font_6x10_tf);

    // Moisture
    display.setCursor(0, 10);
    display.print("M:");
    display.print(moisture, 1);
    display.print("%");

    // pH
    display.setCursor(68, 10);
    display.print("pH:");
    display.print(ph, 1);

    // Temperature
    display.setCursor(0, 22);
    display.print("T:");
    display.print(temperature, 1);
    display.print("C");

    // EC
    display.setCursor(68, 22);
    display.print("EC:");
    display.print(ec);

    // Nitrogen
    display.setCursor(0, 34);
    display.print("N:");
    display.print(nitrogen);

    // Phosphorus
    display.setCursor(68, 34);
    display.print("P:");
    display.print(phosphorus);

    // Potassium
    display.setCursor(0, 46);
    display.print("K:");
    display.print(potassium);

    display.setCursor(68, 46);
    display.print("SOIL");

    display.setCursor(0, 61);
    display.print("RS485 Connected");

    display.sendBuffer();
}

// =====================================================
// OLED Error
// =====================================================

void displaySensorError()
{
    display.clearBuffer();

    display.setFont(u8g2_font_6x10_tf);

    display.setCursor(10, 20);
    display.print("SOIL SENSOR");

    display.setCursor(15, 35);
    display.print("NOT FOUND");

    display.setCursor(5, 50);
    display.print("Check RS485");

    display.sendBuffer();
}

// =====================================================
// Read Soil Sensor
// =====================================================

void readSensor()
{
    // Clear old data
    while (Serial2.available())
    {
        Serial2.read();
    }

    // Enable RS485 transmit
    digitalWrite(RE_DE, HIGH);

    delay(10);

    // Send Modbus request
    Serial2.write(requestData, sizeof(requestData));
    Serial2.flush();

    // Enable RS485 receive
    digitalWrite(RE_DE, LOW);

    // Wait for response
    delay(300);

    if (Serial2.available() >= 19)
    {
        Serial2.readBytes(responseData, 19);

        // Decode sensor data

        moisture =
            ((responseData[3] << 8) | responseData[4]) / 10.0;

        temperature =
            ((responseData[5] << 8) | responseData[6]) / 10.0;

        ec =
            ((responseData[7] << 8) | responseData[8]);

        ph =
            ((responseData[9] << 8) | responseData[10]) / 10.0;

        nitrogen =
            ((responseData[11] << 8) | responseData[12]);

        phosphorus =
            ((responseData[13] << 8) | responseData[14]);

        potassium =
            ((responseData[15] << 8) | responseData[16]);

        // Serial Monitor

        Serial.println();
        Serial.println("================================");

        Serial.printf(
            "Moisture: %.1f %%\n",
            moisture
        );

        Serial.printf(
            "Temperature: %.1f C\n",
            temperature
        );

        Serial.printf(
            "EC: %d\n",
            ec
        );

        Serial.printf(
            "pH: %.1f\n",
            ph
        );

        Serial.printf(
            "Nitrogen: %d\n",
            nitrogen
        );

        Serial.printf(
            "Phosphorus: %d\n",
            phosphorus
        );

        Serial.printf(
            "Potassium: %d\n",
            potassium
        );

        Serial.println("================================");

        // Send to Blynk

        Blynk.virtualWrite(V0, moisture);
        Blynk.virtualWrite(V1, temperature);
        Blynk.virtualWrite(V2, ec);
        Blynk.virtualWrite(V3, ph);
        Blynk.virtualWrite(V4, nitrogen);
        Blynk.virtualWrite(V5, phosphorus);
        Blynk.virtualWrite(V6, potassium);

        // Update OLED

        updateDisplay();
    }
    else
    {
        Serial.println();
        Serial.println("No Response From Soil Sensor");

        displaySensorError();
    }
}

// =====================================================
// Setup
// =====================================================

void setup()
{
    // Serial Monitor
    Serial.begin(115200);

    // RS485

    pinMode(RE_DE, OUTPUT);

    digitalWrite(RE_DE, LOW);

    Serial2.begin(
        4800,
        SERIAL_8N1,
        RXD2,
        TXD2
    );

    // OLED

    Wire.begin(
        OLED_SDA,
        OLED_SCL
    );

    display.begin();

    display.clearBuffer();

    display.setFont(u8g2_font_6x10_tf);

    display.setCursor(20, 20);
    display.print("SOIL MONITOR");

    display.setCursor(25, 35);
    display.print("Starting...");

    display.sendBuffer();

    delay(2000);

    // WiFi / Blynk

    display.clearBuffer();

    display.setCursor(10, 25);
    display.print("Connecting WiFi...");

    display.sendBuffer();

    Blynk.begin(
        BLYNK_AUTH_TOKEN,
        ssid,
        pass
    );

    // Read sensor every 5 seconds

    timer.setInterval(
        5000L,
        readSensor
    );

    Serial.println();
    Serial.println("================================");
    Serial.println("SOIL MONITORING SYSTEM");
    Serial.println("ESP32S + RS485 + 1.3 OLED");
    Serial.println("================================");
}

// =====================================================
// Loop
// =====================================================

void loop()
{
    Blynk.run();
    timer.run();
}