/*    TODO
1. reconstruct the code into class style(abstarct the functions to a class)
2. implement related contents with curtain_state
3. change delay to other way
4. configable wifi ? 
*/

/*改进与完善：
1. 用timer来代替delay，这样可以在拉起或拉下窗帘的时候做其他事情
2. 可以配置改变连接的wifi
3. 实现根据窗帘的状态来控制窗帘

*/

#pragma region Preparation

#include <Arduino.h>
#include <functional>

#define BLINKER_WIFI
#include <Blinker.h>

char auth[] = "b06b42bba5c3";
char ssid[] = "WIFI_C912";
char pswd[] = "17705003103";//配置wifi

#pragma endregion Preparation

#pragma region CP dec
class CurtainPuller
{
private:
    enum PIN //tb6612 使用的引脚和nodeMCU的引脚对应关系
    {
        PWMA = A17,
        AIN2 = A16,
        AIN1 = A15,
        STBY = A14
    };

    int time_pullup; // seconds，拉起窗帘所需时间
    double pulldown_coef;//拉下窗帘乘的系数，因为拉下窗帘的速度比拉起窗帘的速度快
    char motion_state;    // h for halt,u for pullup,d for pulldn，现在是拉起还是拉下，还是停止，用于防止同时操作
    double curtain_state; // 1 for fully closed, 0 for fully opened，窗帘的状态，既打开的程度，因为没法直接测量窗帘的位置，所以用这个变量来模拟
    bool use_led;//是否使用板载led灯
    struct Blk
    {
        BlinkerButton btn_up;
        BlinkerButton btn_dn;
        BlinkerButton btn_dbg;

        Blk() : btn_up("btn_up"), btn_dn("btn_dn"), btn_dbg("btn_dbg") {}//初始化按钮
        ~Blk() = default;

    } blk;

    static CurtainPuller *instance;

    void _btn_up(const String &state);//按钮拉起窗帘的回调函数
    void _btn_dn(const String &state);//按钮拉下窗帘的回调函数
    void _btn_dbg(const String &state);//按钮调试的回调函数
    static void _btn_up_wrapper(const String& state);//按钮拉起窗帘的回调函数的包装函数，成员函数不能直接作为回调函数，所以需要一个包装函数
    static void _btn_dn_wrapper(const String& state);//按钮拉下窗帘的回调函数的包装函数
    static void _btn_dbg_wrapper(const String& state);//按钮调试的回调函数的包装函数

    inline void halt();//停止窗帘
    inline void pullup();//拉起窗帘
    inline void pulldn();//拉下窗帘

    void _delay(bool is_pullingup, double percentage = 1.0);//延时函数，用于模拟窗帘拉起和拉下的时间，percentage是拉起或拉下的百分比

public:
    CurtainPuller() : blk(), time_pullup(90), pulldown_coef(0.9), motion_state('h'), curtain_state(1.0), use_led(true)//初始化
    {
        instance = this;
    }
    ~CurtainPuller() = default;

    void init();//CurtainPuller的初始化函数
    void attach();//类的绑定按钮的回调函数
};
using CP = CurtainPuller;

CP cp;

#pragma region Setup&Loop

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);//波特率为115200
  BLINKER_DEBUG.stream(Serial);
  BLINKER_DEBUG.debugAll();

  cp.init();
  Blinker.begin(auth, ssid, pswd);
  cp.attach();
}

void loop()
{
  // put your main code here, to run repeatedly:
  Blinker.run();
}

#pragma endregion Setup&Loop

#pragma region CP def

CP* CP::instance = nullptr;

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
}
void CP::halt()
{
    if (motion_state == 'h')
        return;
    motion_state = 'h';
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);
    if (use_led)
        digitalWrite(LED_BUILTIN, LOW);
    BLINKER_LOG("halt");
}
void CP::pullup()
{
    if (motion_state != 'h')
    {
        BLINKER_LOG("the motor is working!!!");
        return;
    }
    motion_state = 'u';

    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    if (use_led)
        digitalWrite(LED_BUILTIN, HIGH);
    BLINKER_LOG("pullup");
}
void CP::pulldn()
{
    if (motion_state != 'h')
    {
        BLINKER_LOG("the motor is working!!!");
        return;
    }
    motion_state = 'd';

    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
    if (use_led)
        digitalWrite(LED_BUILTIN, HIGH);
    BLINKER_LOG("pulldn");
}
void CP::_delay(bool is_pullingup, double percentage)
{
    int time_ = time_pullup * 1000 * percentage;
    if (!is_pullingup)
        time_ *= pulldown_coef;
    delay(time_);
    curtain_state += -(2 * is_pullingup - 1) * percentage;
}

void CP::_btn_up(const String &state)
{
    if (state == "tap")
    {
        pullup();
        _delay(true);
        halt();
    }
    else if (state == "press")
    {
        pullup();
    }
    else if (state == "pressup")
    {
        halt();
    }
    else
    {
        BLINKER_LOG("no such state!");
    }
}

void CP::_btn_dn(const String &state)
{
    if (state == "tap")
    {
        pulldn();
        _delay(false);
        halt();
    }
    else if (state == "press")
    {
        pulldn();
    }
    else if (state == "pressup")
    {
        halt();
    }
    else
    {
        BLINKER_LOG("no such state!");
    }
}

void CP::_btn_dbg(const String &state)
{

    BLINKER_LOG("get button state: ", state);
    BLINKER_LOG("DEBUG_INFO:-----------------------------------------------------------");
#if false
  String msg = "";
  if (digitalPinIsValid(PWMA))
  {
    msg += "\nPWMA is valid\n";
  }
  else
  {
    msg += "PWMA is not valid\n";
  }
  if (digitalPinIsValid(STBY))
  {
    msg += "STBY is valid\n";
  }
  else
  {
    msg += "STBY is not valid\n";
  }
  if (digitalPinIsValid(AIN1))
  {
    msg += "AIN1 is valid\n";
  }
  else
  {
    msg += "AIN1 is not valid\n";
  }
  if (digitalPinIsValid(AIN2))
  {
    msg += "AIN2 is valid\n";
  }
  else
  {
    msg += "AIN2 is not valid\n";
  }
  if (digitalPinCanOutput(PWMA))
  {
    msg += "PWMA can output\n";
  }
  else
  {
    msg += "PWMA can not output\n";
  }
  if (digitalPinCanOutput(STBY))
  {
    msg += "STBY can output\n";
  }
  else
  {
    msg += "STBY can not output\n";
  }
  if (digitalPinCanOutput(AIN1))
  {
    msg += "AIN1 can output\n";
  }
  else
  {
    msg += "AIN1 can not output\n";
  }
  if (digitalPinCanOutput(AIN2))
  {
    msg += "AIN2 can output\n";
  }
  else
  {
    msg += "AIN2 can not output\n";
  }

    BLINKER_LOG("DEBUG_INFO:", msg);
#endif
    BLINKER_LOG("DEBUG_INFO:this function is still preparing...");
    BLINKER_LOG("DEBUG_INFO:-----------------------------------------------------------");
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
#pragma endregion CP