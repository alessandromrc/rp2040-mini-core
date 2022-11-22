#include <stdio.h>
#include <cstring>
#include <math.h>
#include "pico/stdlib.h"   // stdlib
#include "hardware/irq.h"  // interrupts
#include "hardware/pwm.h"  // pwm
#include "hardware/sync.h" // wait for interrupt
#include "hardware/resets.h"
#include "hardware/uart.h"
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/divider.h" // hardware divider
#include "pico/multicore.h"   // fifo multicore
#include "hardware/clocks.h"  // clocks
#include "hardware/exception.h"

#define AIRCR_Register (*((volatile uint32_t *)(PPB_BASE + 0x0ED0C)))
#define HIGH true
#define LOW false
#define INPUT 1
#define OUTPUT 2
#define INPUT_PULLUP 3
#define INPUT_PULLDOWN 4
#define ANALOG 5

// Notes to frequency

#define NOTE_B0 31
#define NOTE_C1 33
#define NOTE_CS1 35
#define NOTE_D1 37
#define NOTE_DS1 39
#define NOTE_E1 41
#define NOTE_F1 44
#define NOTE_FS1 46
#define NOTE_G1 49
#define NOTE_GS1 52
#define NOTE_A1 55
#define NOTE_AS1 58
#define NOTE_B1 62
#define NOTE_C2 65
#define NOTE_CS2 69
#define NOTE_D2 73
#define NOTE_DS2 78
#define NOTE_E2 82
#define NOTE_F2 87
#define NOTE_FS2 93
#define NOTE_G2 98
#define NOTE_GS2 104
#define NOTE_A2 110
#define NOTE_AS2 117
#define NOTE_B2 123
#define NOTE_C3 131
#define NOTE_CS3 139
#define NOTE_D3 147
#define NOTE_DS3 156
#define NOTE_E3 165
#define NOTE_F3 175
#define NOTE_FS3 185
#define NOTE_G3 196
#define NOTE_GS3 208
#define NOTE_A3 220
#define NOTE_AS3 233
#define NOTE_B3 247
#define NOTE_C4 262
#define NOTE_CS4 277
#define NOTE_D4 294
#define NOTE_DS4 311
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_FS4 370
#define NOTE_G4 392
#define NOTE_GS4 415
#define NOTE_A4 440
#define NOTE_AS4 466
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_CS5 554
#define NOTE_D5 587
#define NOTE_DS5 622
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_FS5 740
#define NOTE_G5 784
#define NOTE_GS5 831
#define NOTE_A5 880
#define NOTE_AS5 932
#define NOTE_B5 988
#define NOTE_C6 1047
#define NOTE_CS6 1109
#define NOTE_D6 1175
#define NOTE_DS6 1245
#define NOTE_E6 1319
#define NOTE_F6 1397
#define NOTE_FS6 1480
#define NOTE_G6 1568
#define NOTE_GS6 1661
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_CS7 2217
#define NOTE_D7 2349
#define NOTE_DS7 2489
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_FS7 2960
#define NOTE_G7 3136
#define NOTE_GS7 3322
#define NOTE_A7 3520
#define NOTE_AS7 3729
#define NOTE_B7 3951
#define NOTE_C8 4186
#define NOTE_CS8 4435
#define NOTE_D8 4699
#define NOTE_DS8 4978

namespace PICO {

class String {
  bool contains(const char *w1, const char *w2) {
    int i = 0;
    int j = 0;

    for (i; i < strlen(w1); i++) {
      if (w1[i] == w2[j]) {
        j++;
      }
    }

    if (strlen(w2) == j)
      return true;
    else
      return false;
  }
};

class Math {
private:
public:
  static int32_t divide(int32_t a, int32_t b) {
    if (a == 0 || b == 0)
      return -1;
    hw_divider_state_t state;
    hw_divider_divmod_s32_start(a, b);
    hw_divider_save_state(&state);
    hw_divider_restore_state(&state);
    return hw_divider_s32_quotient_wait();
  }

  static int32_t multiply(int32_t a, int32_t b) {
    asm("mul %0, %1" : "+l"(a) : "l"(b) :);
    return a;
  }

  static int32_t add(int32_t a, int32_t b) {
    asm("add %0, %1" : "+l"(a) : "l"(b) :);
    return a;
  }

  static int32_t subtract(int32_t a, int32_t b) {
    asm("sub %0, %1" : "+l"(a) : "l"(b) :);
    return a;
  }

  static double abs(double a) {
    if (a < 0)
      return -a;

    return a;
  }

  static double max(double a, double b) {
    if (a > b)
      return a;
    else
      return b;
  }

  static double min(double a, int32_t b) {
    if (a < b)
      return a;
    else
      return b;
  }

  static double floor(double a) { return (int32_t)(a - 0.5); }

  static double ceil(double a) { return (int32_t)(a + 0.5); }

  static double round(double a) {
    if (a < 0.0)
      return floor(a);
    else
      return ceil(a);
  }

  static int32_t logic_shift_right(int32_t a, int32_t b) {
    asm("LSR %0, %0, %1" : "+l"(a) : "l"(b) :);
    return a;
  }

  static int32_t logic_shift_left(int32_t a, int32_t b) {
    asm("LSL %0, %0, %1" : "+l"(a) : "l"(b) :);
    return a;
  }

  static int32_t arithmetic_shift_right(int32_t a, int32_t b) {
    asm("ASR %0, %0, %1" : "+l"(a) : "l"(b) :);
    return a;
  }

  static int32_t rotate_right(int32_t a, int32_t b) {
    asm("ROR %0, %0, %1" : "+l"(a) : "l"(b) :);
    return a;
  }
};

double map(double input, double input_start, double input_end,
           double output_start, double output_end) {
  return (input - input_start) * (output_end - output_start) /
             (input_end - input_start) +
         output_start;
}

unsigned long millis() { return (unsigned long)time_us_32() / 1000; }

unsigned long micros() { return (unsigned long)time_us_32(); }

void delay(int time) { sleep_ms(time); }

void delayMicroseconds(int time) { sleep_us(time); }

void pinMode(uint pin, int mode) {
  gpio_init(pin);
  if (mode == INPUT)
    gpio_set_dir(pin, GPIO_IN);
  else if (mode == OUTPUT)
    gpio_set_dir(pin, GPIO_OUT);
  else if (mode == INPUT_PULLUP) {
    gpio_set_dir(pin, GPIO_IN);
    gpio_set_pulls(pin, true, false);
  } else if (mode == INPUT_PULLDOWN) {
    gpio_set_dir(pin, GPIO_IN);
    gpio_set_pulls(pin, false, true);
  } else if (mode == ANALOG) {
    adc_gpio_init(pin);
    adc_select_input(0);
  }
}

void initializeADC() { adc_init(); }

int analogReadRaw(uint pin) {
  if (pin == 26) {
    adc_select_input(0);
  } else if (pin == 27) {
    adc_select_input(1);
  } else if (pin == 28) {
    adc_select_input(2);
  }

  return adc_read();
}

float analogReadVoltage(uint pin) {
  return analogReadRaw(pin) * (3.3f / (1 << 12));
}

int analogRead(uint pin) {
  return (int)(map(analogReadRaw(pin), 0, 4095, 0, 1023));
}

void digitalWrite(uint pin, int value) {
  if (value == HIGH)
    gpio_put(pin, HIGH);
  else if (value == LOW)
    gpio_put(pin, LOW);
}

bool clockWrite(uint pin, uint divider) {
  switch (pin) {
  case 21:
  case 23:
  case 24:
  case 26:
    clock_gpio_init(pin, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, divider);
    return true;
    break;
  default:
    return false;
    break;
  }
}

bool writeFrequency(uint pin, float frequency) {
  return clockWrite(pin, Math::divide(clock_get_hz(clk_sys), frequency));
}

void analogWrite(uint pin, float value) { gpio_put(pin, value); }

bool digitalRead(uint pin) { return gpio_get(pin); }

void pinFunction(uint pin, gpio_function function) {
  gpio_set_function(pin, function);
}

class Sys {
public:
  void reset() { AIRCR_Register = 0x5FA0004; }

  void nop() {
    asm("MOV R0, R0" :);
  } // funny way of doing a nop, move register zero to itself :)

  uint8_t chip_version() { return rp2040_chip_version(); }

  uint8_t rom_version() { return rp2040_rom_version(); }
};

class Serial {
private:
  uart_inst *UART_ID = 0;

public:
  void Begin(uart_inst *uartID, int Baudrate, int txPin, int rxPin) {
    uart_init(uartID, Baudrate);
    UART_ID = uartID;
    pinFunction(txPin, GPIO_FUNC_UART);
    pinFunction(rxPin, GPIO_FUNC_UART);
  }

  void writeRaw(char character) { uart_putc_raw(UART_ID, character); }

  void putc(char character) { uart_putc(UART_ID, character); }

  void writeString(const char *string) { uart_puts(UART_ID, string); }

  uart_inst *getUartID() { return UART_ID; }
};

class Midi {
private:
  Serial serial;
  enum MidiType : uint8_t {
    InvalidType = 0x00,    ///< For notifying errors
    NoteOff = 0x80,        ///< Channel Message - Note Off
    NoteOn = 0x90,         ///< Channel Message - Note On
    AfterTouchPoly = 0xA0, ///< Channel Message - Polyphonic AfterTouch
    ControlChange = 0xB0,  ///< Channel Message - Control Change / Channel Mode
    ProgramChange = 0xC0,  ///< Channel Message - Program Change
    AfterTouchChannel =
        0xD0,         ///< Channel Message - Channel (monophonic) AfterTouch
    PitchBend = 0xE0, ///< Channel Message - Pitch Bend
    SystemExclusive = 0xF0,                 ///< System Exclusive
    SystemExclusiveStart = SystemExclusive, ///< System Exclusive Start
    TimeCodeQuarterFrame =
        0xF1,            ///< System Common - MIDI Time Code Quarter Frame
    SongPosition = 0xF2, ///< System Common - Song Position Pointer
    SongSelect = 0xF3,   ///< System Common - Song Select
    Undefined_F4 = 0xF4,
    Undefined_F5 = 0xF5,
    TuneRequest = 0xF6,        ///< System Common - Tune Request
    SystemExclusiveEnd = 0xF7, ///< System Exclusive End
    Clock = 0xF8,              ///< System Real Time - Timing Clock
    Undefined_F9 = 0xF9,
    Tick = Undefined_F9, ///< System Real Time - Timing Tick (1 tick = 10
    ///< milliseconds)
    Start = 0xFA,    ///< System Real Time - Start
    Continue = 0xFB, ///< System Real Time - Continue
    Stop = 0xFC,     ///< System Real Time - Stop
    Undefined_FD = 0xFD,
    ActiveSensing = 0xFE, ///< System Real Time - Active Sensing
    SystemReset = 0xFF,   ///< System Real Time - System Reset
  };

public:
  void Begin(uart_inst *uartID, int txPin, int rxPin) {
    serial.Begin(uartID, 31250, txPin, rxPin);
  }

  void sendNoteOn(int pitch, int velocity) {
    serial.writeRaw(MidiType::NoteOn);
    serial.writeRaw(pitch);
    serial.writeRaw(velocity);
  }

  void sendNoteOff(int pitch, int velocity) {
    serial.writeRaw(MidiType::NoteOff);
    serial.writeRaw(pitch);
    serial.writeRaw(velocity);
  }

  void sendAftertouchPoly(int key, int touch) {
    serial.writeRaw(MidiType::AfterTouchPoly);
    serial.writeRaw(key);
    serial.writeRaw(touch);
  }

  void sendControlChange(int controller, int value) {
    serial.writeRaw(MidiType::ControlChange);
    serial.writeRaw(controller);
    serial.writeRaw(value);
  }

  void patchChange(int instrument) {
    serial.writeRaw(MidiType::ControlChange);
    serial.writeRaw(instrument);
  }

  void channelPressure(int pressure) {
    serial.writeRaw(MidiType::AfterTouchChannel);
    serial.writeRaw(pressure);
  }

  void pitchBend(int bend_lsb, int bend_msb) {
    serial.writeRaw(MidiType::PitchBend);
    serial.writeRaw(bend_lsb);
    serial.writeRaw(bend_msb);
  }

  void sendInvalidType() { serial.writeRaw(MidiType::InvalidType); }

  void systemReset() { serial.writeRaw(MidiType::SystemReset); }

  void activeSensing() { serial.writeRaw(MidiType::ActiveSensing); }
};

class USB_SERIAL {
private:
  char buf[128];

public:
  static void print(const char *str) { printf("%s", str); }

  static void print(int value) { printf("%d", value); }

  static void print(unsigned int value) { printf("%u", value); }

  static void print(long value) { printf("%ld", value); }

  static void print(unsigned long value) { printf("%lu", value); }

  static void print(float value) { printf("%f", value); }

  static void print(double value) { printf("%f", value); }

  static void print(char c) { printf("%c", c); }

  static void println(const char *str) { printf("%s\n", str); }

  static void println(int value) { printf("%d\n", value); }

  static void println(unsigned int value) { printf("%u\n", value); }

  static void println(long value) { printf("%ld\n", value); }

  static void println(unsigned long value) { printf("%lu\n", value); }

  static void println(float value) { printf("%f\n", value); }

  static void println(double value) { printf("%f\n", value); }

  static void println() { printf("\n"); }

  static void println(char c) { printf("%c\n", c); }

  char getChar() {
    char c = getchar_timeout_us(0);

    if (c > 0 && c < 127) {
      return c;
    }

    return 0;
  }

  const char *getString() {
    char c;
    int i = 0;
    while ((c = getChar()) != '\n' && c != '\r' && c != 0) {
      buf[i++] = c;
    }

    buf[i] = 0;
    return buf;
  }

  void flush() { stdio_flush(); }
};

class Temperature {
private:
  float read_onboard_temperature(const char unit) {
    /*12-bit conversion, assume max value == ADC_VREF == 3.3 V */
    const float conversionFactor = 3.3f / (1 << 12);

    float adc = (float)adc_read() * conversionFactor;
    float tempC = 27.0f - (adc - 0.706f) / 0.001721f;

    if (unit == 'C') {
      return tempC;
    } else if (unit == 'F') {
      return tempC * 9 / 5 + 32;
    }

    return -1.0f;
  }

public:
  void Begin() { adc_set_temp_sensor_enabled(true); }

  float read(const char unit) {
    adc_select_input(4);
    return read_onboard_temperature(unit);
  }
};

class Interrupt {
public:
  enum states : uint32_t {
    low = GPIO_IRQ_LEVEL_LOW,       // 1
    high = GPIO_IRQ_LEVEL_HIGH,     // 2
    edge_fall = GPIO_IRQ_EDGE_FALL, // 4
    edge_rise = GPIO_IRQ_EDGE_RISE, // 8
    on_change = 0xC                 // 12
  };
  void attachInterrupt(uint gpio, uint32_t mask, bool enabled,
                       gpio_irq_callback_t callback) {
    gpio_set_irq_enabled(gpio, mask, enabled);
    gpio_set_irq_callback(callback);
    irq_set_enabled(IO_IRQ_BANK0, enabled);
  }

  const char *getStateString(uint32_t event) {
    switch (event) {
    case states::low:
      return "LEVEL_LOW";
      break;
    case states::high:
      return "LEVEL_HIGH";
      break;
    case states::edge_fall:
      return "EDGE_FALL";
      break;
    case states::edge_rise:
      return "EDGE_RISE";
      break;
    case states::on_change:
      return "ON_CHANGE";
      break;
    default:
      return "UNKNOWN";
      break;
    }
  }
};

class Encoder {
private:
  Interrupt interrupt;
  int lastMSB = 0;
  int lastLSB = 0;
  volatile long encoderValue = 0;
  volatile int lastEncoded = 0;
  uint upper_pin = 0;
  uint lower_pin = 0;
  bool changed = false;

public:
  int getValue() { return encoderValue; }
  void reset() { encoderValue = 0; }
  void update(uint gpio) {
    if ((gpio == upper_pin) || (gpio == lower_pin)) {
      int MSB = digitalRead(upper_pin);
      int LSB = digitalRead(lower_pin);
      int encoded = (MSB << 1) | LSB;
      int sum = (lastEncoded << 2) | encoded;
      if (sum == 0b1000)
        encoderValue++;
      if (sum == 0b0010)
        encoderValue--;
      changed = true;
      lastEncoded = encoded;
    }
  }
  bool hasChanged() {
    if (changed) {
      changed = false;
      return true;
    }
    return false;
  }
  Encoder(const uint EncoderPin[], gpio_irq_callback_t callback) {
    upper_pin = EncoderPin[0];
    lower_pin = EncoderPin[1];
    pinMode(upper_pin, INPUT_PULLUP);
    pinMode(lower_pin, INPUT_PULLUP);
    interrupt.attachInterrupt(upper_pin, interrupt.states::on_change, true,
                              callback);
    interrupt.attachInterrupt(lower_pin, interrupt.states::on_change, true,
                              callback);
  }
};

class Core {
private:
  static void core1_entry() {
    while (1) {
      int32_t (*func)(int32_t) =
          (int32_t (*)(int32_t))multicore_fifo_pop_blocking();
      int32_t p = (int32_t)multicore_fifo_pop_blocking();
      int32_t result = (*func)(p);
      multicore_fifo_push_blocking(result);
    }
  }

public:
  void sync_push(uint32_t data) { multicore_fifo_push_blocking(data); }

  bool async_push(uint32_t data) // return true if data got sent as the core was
                                 // free otherwise return false!
  {
    if (multicore_fifo_wready()) {
      multicore_fifo_push_blocking(data);
      return true;
    } else {
      return false;
    }
  }

  bool pushTimeout(uint32_t data, uint64_t timeout_us) {
    return multicore_fifo_push_timeout_us(data, timeout_us);
  }

  uint32_t sync_pop() { return multicore_fifo_pop_blocking(); }

  uint32_t async_pop() {
    if (multicore_fifo_rvalid()) {
      return multicore_fifo_pop_blocking();
    }
  }

  void nop_tight_loop() { tight_loop_contents(); }

  void launch() { multicore_launch_core1(core1_entry); }

  void launch_task(void (*task)(void)) { multicore_launch_core1(task); }
};

class Exception {
private:
public:
  uint get_current_exception() { return __get_current_exception(); }

  exception_handler_t get_vtable_handler(enum exception_number num) {
    return exception_get_vtable_handler(num);
  }

  void restore_handler(enum exception_number num,
                       exception_handler_t original_handler) {
    exception_restore_handler(num, original_handler);
  }

  void set_exclusive_handler(enum exception_number num,
                             exception_handler_t handler) {
    exception_set_exclusive_handler(num, handler);
  }
};

class Debouncing {
private:
  int btn;
  int curState, prevState, debState;
  unsigned long curMillis;
  unsigned long prevMillis;
  unsigned long debounceMillis = 20;
  unsigned long curClickMillis;
  unsigned long prevClickMillis;
  unsigned long clickTimerOffMillis = 1;
  unsigned long curReleaseMillis;
  unsigned long prevReleaseMillis;
  unsigned long releaseTimerOffMillis = 0;
  bool pressedState = false;
  bool toggleState = false;
  bool pressing = false;
  bool clicking = false;
  bool releasing = false;
  bool released = true;
  int pinModeState;

public:
  Debouncing(int button, int mode) {
    if (mode == INPUT) {
      curState = HIGH;
      prevState = LOW;
      debState = LOW;
    } else {
      curState = LOW;
      prevState = LOW;
      debState = HIGH;
    }
    btn = button;
    PICO::pinMode(btn, mode);
    pinModeState = mode;
    prevMillis = PICO::millis();
  }
  bool press() {
    curMillis = PICO::millis();
    curState = PICO::digitalRead(btn);
    if (curState != prevState) {
      prevMillis = curMillis;
    }
    if ((unsigned long)curMillis - prevMillis > debounceMillis) {
      if (curState != debState) {
        debState = curState;
        if (pinModeState == INPUT) {
          if (debState == HIGH) {
            pressedState = true;
          } else {
            pressedState = false;
          }
        } else {
          if (debState == LOW) {
            pressedState = true;
          } else {
            pressedState = false;
          }
        }
      }
      prevMillis = curMillis;
    }
    prevState = curState;
    return pressedState;
  }
  bool toggle() {
    if (press()) {
      if (!pressing) {
        toggleState = !toggleState;
        pressing = true;
      }
    } else {
      pressing = false;
    }
    return toggleState;
  }

  bool click() {
    curClickMillis = PICO::millis();
    if ((unsigned long)curClickMillis - prevClickMillis > clickTimerOffMillis &&
        !released && clicking) {
      clicking = false;
      prevClickMillis = curClickMillis;
    }
    if (press()) {
      if (released) {
        clicking = true;
        released = false;
        prevClickMillis = curClickMillis;
      }
    } else {
      released = true;
    }
    return clicking;
  }
  bool release() {
    curReleaseMillis = PICO::millis();
    if ((unsigned long)curReleaseMillis - prevReleaseMillis >
            releaseTimerOffMillis &&
        released && !releasing) {
      releasing = true;
      prevReleaseMillis = curReleaseMillis;
    }
    if (press()) {
      if (!released) {
        releasing = false;
        released = true;
        prevReleaseMillis = curReleaseMillis;
      }
    } else {
      released = false;
    }
    return releasing;
  }
};

class Tone {
private:
  unsigned long currentTime = 0;
  unsigned long previousTime = 0;
  uint choosen_pin = 0;

public:
  void update() { currentTime = millis(); }

  void stop() {
    switch (choosen_pin) {
    case 21:
      clock_stop(clk_gpout0);
      break;
    case 23:
      clock_stop(clk_gpout1);
      break;
    case 24:
      clock_stop(clk_gpout2);
      break;
    case 26:
      clock_stop(clk_gpout3);
      break;
    }
  }

  void play(float frequency, float duration) {
    if (currentTime - previousTime < duration) {
      writeFrequency(choosen_pin, frequency);
    } else {
      stop();
      previousTime = currentTime;
    }
  }

  Tone(uint pin) { choosen_pin = pin; }
};

Midi midi;
Sys sys;
USB_SERIAL usb_serial;
Temperature temp;
Core core;
Interrupt interrupt;
Exception exception;
Math math;
}

using namespace PICO;

// boot init
const uint switch_reset = 2;
Debouncing Reset(switch_reset, INPUT_PULLUP);
