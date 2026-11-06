#include "AngleTracker.h"
#include "config.h"
#include <Wire.h>

namespace {

// ---- LSM6 family registers (common to LSM6DS3/DS3TR-C/DSO/DSOX/ISM330DHCX) ----
constexpr uint8_t REG_WHO_AM_I = 0x0F;
constexpr uint8_t REG_CTRL1_XL = 0x10;
constexpr uint8_t REG_CTRL2_G  = 0x11;
constexpr uint8_t REG_CTRL3_C  = 0x12;
constexpr uint8_t REG_OUTX_L_G = 0x22;

constexpr uint8_t WHO_LSM6DS3   = 0x69;
constexpr uint8_t WHO_LSM6DS3TR = 0x6A;  // also LSM6DSL
constexpr uint8_t WHO_ISM330    = 0x6B;
constexpr uint8_t WHO_LSM6DSO   = 0x6C;  // also LSM6DSOX

uint8_t imuAddr = 0;
float   radPerLsb = 0;
float   biasLsb = 0;

portMUX_TYPE stateMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE hallMux  = portMUX_INITIALIZER_UNLOCKED;
WheelState shared = {};

volatile uint32_t hallTimeUs = 0;
volatile uint32_t hallLastAcceptedUs = 0;
volatile bool     hallFlag = false;

bool writeReg(uint8_t reg, uint8_t v) {
  Wire.beginTransmission(imuAddr);
  Wire.write(reg);
  Wire.write(v);
  return Wire.endTransmission() == 0;
}

bool readRegs(uint8_t reg, uint8_t* buf, size_t n) {
  Wire.beginTransmission(imuAddr);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint16_t)imuAddr, n, true) != n) return false;
  for (size_t i = 0; i < n; i++) buf[i] = Wire.read();
  return true;
}

bool readGyroAxle(int16_t& raw) {
  uint8_t b[6];
  if (!readRegs(REG_OUTX_L_G, b, 6)) return false;
  raw = (int16_t)(b[2 * GYRO_AXLE_AXIS] | (b[2 * GYRO_AXLE_AXIS + 1] << 8));
  return true;
}

bool initImu() {
  const uint8_t addrs[] = {0x6A, 0x6B};
  uint8_t who = 0;
  for (uint8_t a : addrs) {
    imuAddr = a;
    if (readRegs(REG_WHO_AM_I, &who, 1) &&
        (who == WHO_LSM6DS3 || who == WHO_LSM6DS3TR || who == WHO_ISM330 || who == WHO_LSM6DSO)) {
      break;
    }
    who = 0;
  }
  if (!who) return false;
  Serial.printf("IMU at 0x%02X, WHO_AM_I=0x%02X\n", imuAddr, who);

  writeReg(REG_CTRL3_C, 0x01);  // software reset
  delay(20);
  writeReg(REG_CTRL3_C, 0x44);  // BDU + auto-increment
  writeReg(REG_CTRL1_XL, 0x84); // accel 1.66 kHz, ±16 g (kept on for future gravity fallback)

  float mdpsPerLsb;
  if (GYRO_FS_DPS == 4000 && who == WHO_ISM330) {
    writeReg(REG_CTRL2_G, 0x81);  // 1.66 kHz, ±4000 dps (FS_4000 bit)
    mdpsPerLsb = 140.0f;
  } else {
    writeReg(REG_CTRL2_G, 0x8C);  // 1.66 kHz, ±2000 dps
    mdpsPerLsb = 70.0f;
  }
  radPerLsb = mdpsPerLsb * 0.001f * DEG_TO_RAD;
  delay(50);

  // Bias calibration: only accepted if the wheel is still during boot.
  const int N = 400;
  float sum = 0, sumSq = 0;
  int got = 0;
  for (int i = 0; i < N; i++) {
    int16_t r;
    if (readGyroAxle(r)) { sum += r; sumSq += (float)r * r; got++; }
    delay(1);
  }
  if (got > N / 2) {
    float mean = sum / got;
    float sd = sqrtf(fmaxf(sumSq / got - mean * mean, 0));
    if (sd * mdpsPerLsb * 0.001f < 1.0f) {   // < 1 dps noise -> wheel was still
      biasLsb = mean;
    }
    Serial.printf("Gyro bias %.2f dps (sd %.2f dps) %s\n", mean * mdpsPerLsb * 0.001f,
                  sd * mdpsPerLsb * 0.001f, biasLsb != 0 ? "applied" : "ignored (moving)");
  }
  return true;
}

void IRAM_ATTR hallIsr() {
  uint32_t t = micros();
  portENTER_CRITICAL_ISR(&hallMux);
  if (t - hallLastAcceptedUs >= HALL_MIN_PERIOD_US) {
    hallLastAcceptedUs = t;
    hallTimeUs = t;
    hallFlag = true;
  }
  portEXIT_CRITICAL_ISR(&hallMux);
}

inline float wrapPi(float a) {
  while (a > PI) a -= TWO_PI;
  while (a < -PI) a += TWO_PI;
  return a;
}

void trackerTask(void*) {
  float angle = 0;        // wrapped wheel angle
  float sincePulse = 0;   // signed gyro rotation since last Hall pulse (not wrapped)
  float scale = 1.0f;
  float omega = 0;
  float hallRps = 0;
  uint32_t prevHallUs = 0;
  uint32_t hallCount = 0;
  bool haveHall = false;
  const float deadband = GYRO_DEADBAND_DPS * DEG_TO_RAD;

  uint32_t lastUs = micros();
  TickType_t wake = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(TRACKER_PERIOD_MS));

    int16_t raw;
    if (readGyroAxle(raw)) {
      float w = (raw - biasLsb) * radPerLsb * GYRO_SIGN;
      if (fabsf(w) < deadband) w = 0;
      if (abs(raw) > 32000 && hallRps > 0) {
        // Gyro saturated: fall back to the Hall-measured rate.
        w = copysignf(TWO_PI * hallRps / scale, w);
      }
      omega = w * scale;
    }

    uint32_t now = micros();
    float dt = (now - lastUs) * 1e-6f;
    lastUs = now;
    if (dt > 0.05f) dt = 0.05f;
    angle += omega * dt;
    sincePulse += omega * dt;

#if USE_HALL
    bool flag;
    uint32_t hT;
    portENTER_CRITICAL(&hallMux);
    flag = hallFlag;
    hT = hallTimeUs;
    hallFlag = false;
    portEXIT_CRITICAL(&hallMux);

    if (flag) {
      float ago = (int32_t)(now - hT) * 1e-6f;       // pulse happened this long ago
      float angleAtPulse = angle - omega * ago;
      float turned = fabsf(sincePulse - omega * ago); // gyro's estimate of one revolution

      bool fresh = haveHall && (hT - prevHallUs) < HALL_TIMEOUT_US;
      if (fresh) {
        hallRps = 1e6f / (float)(hT - prevHallUs);
        if (turned > 0.7f * TWO_PI && turned < 1.3f * TWO_PI) {  // rejects missed pulses
          scale *= 1.0f + SCALE_LEARN_RATE * (TWO_PI / turned - 1.0f);
          scale = constrain(scale, 0.8f, 1.25f);
        }
      } else {
        hallRps = 0;
      }

      float err = wrapPi(0.0f - angleAtPulse);
      angle += fresh ? HALL_BLEND * err : err;
      sincePulse = omega * ago;
      prevHallUs = hT;
      haveHall = true;
      hallCount++;
    }
    if (haveHall && (now - prevHallUs) > HALL_TIMEOUT_US) hallRps = 0;
#endif

    angle = fmodf(angle, TWO_PI);
    if (angle < 0) angle += TWO_PI;

    portENTER_CRITICAL(&stateMux);
    shared.angle = angle;
    shared.omega = omega;
    shared.timeUs = now;
    shared.gyroScale = scale;
    shared.hallRps = hallRps;
    shared.hallCount = hallCount;
    portEXIT_CRITICAL(&stateMux);
  }
}

}  // namespace

namespace Tracker {

bool begin() {
  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  bool ok = initImu();
  shared.imuOk = ok;
  shared.gyroScale = 1.0f;
  if (!ok) {
    Serial.println("IMU not found!");
    return false;
  }

#if USE_HALL
  pinMode(HALL_PIN, INPUT_PULLUP);
  attachInterrupt(HALL_PIN, hallIsr, HALL_ACTIVE_LOW ? FALLING : RISING);
#endif

  // Core 1, above the Arduino loop task (priority 1) so LED work never delays gyro samples.
  xTaskCreatePinnedToCore(trackerTask, "tracker", 4096, nullptr, 5, nullptr, 1);
  return true;
}

WheelState snapshot() {
  portENTER_CRITICAL(&stateMux);
  WheelState s = shared;
  portEXIT_CRITICAL(&stateMux);
  return s;
}

float predictAngle(const WheelState& s, uint32_t tUs) {
  float dt = (int32_t)(tUs - s.timeUs) * 1e-6f;
  return s.angle + s.omega * dt;
}

}  // namespace Tracker
