#pragma region Preparation

#include <Arduino.h>
#include <functional>
#include <TickTwo.h>

#define BLINKER_WIFI
#include <Blinker.h>

#define DEBUG true

char auth[] = "b06b42bba5c3";

#define wifi_choice 2

#if (wifi_choice == 1)
char ssid[] = "WIFI_C912";
char pswd[] = "17705003103"; // 配置wifi
#endif

#if (wifi_choice == 2)
char ssid[] = "Xe123";
char pswd[] = "91215225825"; // 配置wifi
#endif

#if (wifi_choice == 3)
char ssid[] = "Note12";
char pswd[] = "12345678";
#endif

#pragma endregion Preparation

#pragma region CP dec
class CurtainPuller
{
private:
  enum PIN // tb6612 使用的引脚和nodeMCU的引脚对应关系
  {
    PWMA = A17,
    AIN2 = A16,
    AIN1 = A15,
    STBY = A14
  };

  uint32_t time_pullup; // 拉起窗帘所需时间
  double pulldown_coef; // 拉下窗帘乘的系数，因为拉下窗帘的速度比拉起窗帘的速度快
  char motion_state;    // h for halt,u for pullup,d for pulldn，现在是拉起还是拉下，还是停止，用于防止同时操作
  double curtain_state; // 1 for fully closed, 0 for fully opened，窗帘的状态，既打开的程度，因为没法直接测量窗帘的位置，所以用这个变量来模拟
  bool use_led;         // 是否使用板载led灯
  // hw_timer_t *timer;
  uint32_t start_time;
  TickTwo *timer;

  struct Blk
  {
    BlinkerButton btn_up;
    BlinkerButton btn_dn;
    BlinkerButton btn_dbg;
    BlinkerButton btn_rst;
    BlinkerSlider sld_tg; // target

    Blk() : btn_up("btn_up"), btn_dn("btn_dn"), btn_dbg("btn_dbg"), btn_rst("btn_rst"), sld_tg("ran-tg") {} // 初始化按钮
    ~Blk() = default;

  } blk;

  static CurtainPuller *instance;

  void _btn_up(const String &state);                 // 按钮拉起窗帘的回调函数
  void _btn_dn(const String &state);                 // 按钮拉下窗帘的回调函数
  void _btn_dbg(const String &state);                // 按钮调试的回调函数
  void _btn_rst(const String &state);                // 按钮复位的回调函数
  void _sld_tg(int32_t data);                        //
  static void _btn_up_wrapper(const String &state);  // 按钮拉起窗帘的回调函数的包装函数，成员函数不能直接作为回调函数，所以需要一个包装函数
  static void _btn_dn_wrapper(const String &state);  // 按钮拉下窗帘的回调函数的包装函数
  static void _btn_dbg_wrapper(const String &state); // 按钮调试的回调函数的包装函数
  static void _btn_rst_wrapper(const String &state); // 按钮复位的回调函数的包装函数
  static void _sld_tg_wrapper(int32_t data);         // rgb调试的回调函数的包装函数

  static void halt_wrapper(); // 停止窗帘的回调函数

  inline void halt();                      // 停止窗帘
  void pull(double target_state);          // 拉起或拉下窗帘
  inline void pullup(uint32_t time_diff);  // 拉起窗帘
  inline void pulldn(uint32_t time_diff);  // 拉下窗帘
  void start_timer(uint32_t elapsed_time); // 开始计时
  uint32_t stop_timer();                   // 停止计时

public:
  CurtainPuller() : blk(), time_pullup(100 * 1e3), pulldown_coef(0.9), motion_state('h'),
                    curtain_state(1.0), use_led(true), start_time(0u), timer(nullptr) // 初始化
  {
    instance = this;
  }
  ~CurtainPuller() = default;

  void init();   // CurtainPuller的初始化函数
  void attach(); // 类的绑定按钮的回调函数
  void loop();   // 主循环
};
using CP = CurtainPuller;

CP cp;

#pragma region Setup&Loop

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200); // 波特率为115200
  BLINKER_DEBUG.stream(Serial);
  BLINKER_DEBUG.debugAll();

  cp.init();
  Blinker.begin(auth, ssid, pswd);
  cp.attach();
}

void loop()
{
  // put your main code here, to run repeatedly:
  cp.loop();
  Blinker.run();
}

#pragma endregion Setup &Loop

#pragma region CP def

CP *CP::instance = nullptr;

void CP::init()
{
  pinMode(PWMA, OUTPUT);
  pinMode(STBY, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(PWMA, HIGH);
  digitalWrite(STBY, HIGH);
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(LED_BUILTIN, LOW);
}
void CP::attach()
{
  blk.btn_up.attach(_btn_up_wrapper);
  blk.btn_dn.attach(_btn_dn_wrapper);
  blk.btn_dbg.attach(_btn_dbg_wrapper);
  blk.btn_rst.attach(_btn_rst_wrapper);
  blk.sld_tg.attach(_sld_tg_wrapper);
}

void CP::loop()
{
  if (timer != nullptr)
  {
    timer->update();
  }
}
void CP::halt()
{
  if (motion_state == 'h')
    return;

  uint32_t time_diff = stop_timer();
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  if (use_led)
    digitalWrite(LED_BUILTIN, LOW);

  if (DEBUG)
  {
    BLINKER_LOG("-------------------- in halt -------------------");
    BLINKER_LOG("time_diff: ", time_diff);
    BLINKER_LOG("-------------------- in halt -------------------");
  }
  double state_diff = static_cast<double>(time_diff) / time_pullup;

  switch (motion_state)
  {
  case 'u':
  {
    if (DEBUG)
      BLINKER_LOG("change state diff: ", state_diff);
    curtain_state -= state_diff;
    break;
  }
  case 'd':
    curtain_state += state_diff / pulldown_coef;
    break;
  default:
    break;
  }

  motion_state = 'h';
}
void CP::pull(double target_state)
{
  // if()

  double state_diff = (target_state - curtain_state);
  if (DEBUG)
  {
    BLINKER_LOG("-------------------- in pull -------------------");
    BLINKER_LOG("state diff: ", state_diff);
    BLINKER_LOG("-------------------- in pull -------------------");
  }
  if (state_diff > 0)
  {
    pulldn(state_diff * time_pullup * pulldown_coef);
  }
  else if (state_diff < 0)
  {
    pullup(-state_diff * time_pullup);
  }
  else
  {
    halt(); // TODO
  }
}

void CP::pullup(uint32_t time_diff = 0)
{
  if (DEBUG)
  {
    BLINKER_LOG("-------------------- in pull up -------------------");
    BLINKER_LOG("time_diff: ", time_diff);
    BLINKER_LOG("-------------------- in pull up -------------------");
  }
  if (motion_state != 'h')
    return;

  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, HIGH);
  if (use_led)
    digitalWrite(LED_BUILTIN, HIGH);

  motion_state = 'u';

  if (time_diff == uint32_t(0))
  {
    start_timer(time_pullup);
  }
  else
  {
    start_timer(time_diff);
  }
}
void CP::pulldn(uint32_t time_diff = 0)
{

  BLINKER_LOG("-------------------- in pull down -------------------");
  BLINKER_LOG("time_diff: ", time_diff);
  BLINKER_LOG("-------------------- in pull down -------------------");
  if (motion_state != 'h')
    return;

  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  if (use_led)
    digitalWrite(LED_BUILTIN, HIGH);

  motion_state = 'd';
  if (time_diff == uint32_t(0))
  {
    start_timer(time_pullup * pulldown_coef);
  }
  else
  {
    start_timer(time_diff);
  }
}

void CP::start_timer(uint32_t elapsed_time)
{
  if (timer != nullptr)
    return;
  start_time = millis();
  timer = new TickTwo(halt_wrapper, elapsed_time, 1, MILLIS);
  timer->start();
}

uint32_t CP::stop_timer()
{
  uint32_t time_diff = millis() - start_time;
  if (DEBUG)
  {
    BLINKER_LOG("================================== in stop timer ==================================");
    BLINKER_LOG("time_diff: ", time_diff);
    BLINKER_LOG("================================== in stop timer ==================================");
  }
  timer->stop();
  delete timer;
  timer = nullptr;
  return time_diff;
}

void CP::_btn_up(const String &state)
{
  if (state == "tap")
  {
    pull(0);
  }
  else if (state == "press")
  {
    pullup();
  }
  else // pressup
  {
    halt();
  }
}

void CP::_btn_dn(const String &state)
{
  if (state == "tap")
  {
    pull(1);
  }
  else if (state == "press")
  {
    pulldn();
  }
  else // pressup
  {
    halt();
  }
}

void CP::_btn_dbg(const String &state)
{

  BLINKER_LOG("get button state: ", state);
  BLINKER_LOG("DEBUG_INFO:-----------------------------------------------------------");
  BLINKER_LOG("DEBUG_INFO:curtain_state: ", curtain_state);
  BLINKER_LOG("DEBUG_INFO:motion_state: ", motion_state);
  BLINKER_LOG("DEBUG_INFO:-----------------------------------------------------------");
}

void CP::_btn_rst(const String &state)
{
  if (state == "tap" || state == "press")
  {
    halt();
    motion_state = 'h';
    curtain_state = 1.0;

  }
  else
  {
    if (DEBUG)
    {
      BLINKER_LOG("-------------------- in reset -------------------");
      BLINKER_LOG("tap or press to reset");
      BLINKER_LOG("-------------------- in reset -------------------");
    }
  }
}

void CP::_sld_tg(int32_t data)
{
  if (DEBUG)
    BLINKER_LOG("get slider data: ", data);
  pull((static_cast<double>(data) / 100.0));
}

void CP::_btn_up_wrapper(const String &state)
{
  if (instance)
    instance->_btn_up(state);
}
void CP::_btn_dn_wrapper(const String &state)
{
  if (instance)
    instance->_btn_dn(state);
}
void CP::_btn_dbg_wrapper(const String &state)
{
  if (instance)
    instance->_btn_dbg(state);
}

void CP::_btn_rst_wrapper(const String &state)
{
  if (instance)
    instance->_btn_rst(state);
}

void CP::_sld_tg_wrapper(int32_t data)
{
  if (instance)
    instance->_sld_tg(data);
}

void IRAM_ATTR CP::halt_wrapper()
{
  if (instance)
    instance->halt();
}

#pragma endregion CP