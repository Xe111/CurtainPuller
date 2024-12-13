#pragma region 数据准备区

    #define BLINKER_WIFI
    #include <Blinker.h>
    #include <Arduino.h>
    #include <functional>
    #include <TickTwo.h>
    #define DEBUG true

    char auth[] = "b06b42bba5c3";       // NodeMCU-32S
    //char auth[] = "d25e869afcac";      // ESP32-DevKitC

    #define wifi_choice 4

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

    #if (wifi_choice == 4)
    char ssid[] = "Win11";
    char pswd[] = "12345678";
    #endif

#pragma endregion 数据准备区


#pragma region 全局变量区

    enum PIN { PWMA = A17, AIN2 = A16, AIN1 = A15, STBY = A14 };// tb6612 使用的引脚和nodeMCU的引脚对应关系
    uint32_t time_pullup = 100 * 1e3; // 拉起窗帘所需时间
    double pulldown_coef = 0.9;       // 拉下窗帘乘的系数，因为拉下窗帘的速度比拉起窗帘的速度快
    char motion_state = 'h';          // 窗帘动作状态: h:停机; u:正在上升; d:正在下拉;
    double curtain_state = 1;         // 窗帘打开状态: 0:全开; 1:全关
    bool use_led = true;              // 是否启用LED灯
    uint32_t start_time = 0u;         // 开始时间
    TickTwo *timer = nullptr;         // 计时器

#pragma endregion global_variables


#pragma region 按钮注册和函数声明

    BlinkerButton btn_up("btn_up");
    BlinkerButton btn_dn("btn_dn");
    BlinkerButton btn_dbg("btn_dbg");
    BlinkerButton btn_rst("btn_rst");
    BlinkerButton btn_led("btn_led");
    BlinkerSlider sld_tg("ran_tg");   // 目标设置滑动条
    BlinkerSlider sld_sta("ran_sta"); // 状态显示滑动条
    BlinkerNumber num_sta("num_sta"); // 状态显示数字

    void _btn_up(const String &state);  // 按钮拉起窗帘的回调函数
    void _btn_dn(const String &state);  // 按钮拉下窗帘的回调函数
    void _btn_dbg(const String &state); // 按钮调试的回调函数
    void _btn_rst(const String &state); // 按钮复位的回调函数
    void _btn_led(const String &state); // 按钮复位的回调函数
    void _sld_tg(int32_t data);         // 目标状态滑动条响应函数

    void _data_storage();                   // 历史数据存储函数, 每分钟执行一次
    void _rtData();                         // 实时数据上传函数, 每秒执行一次
    void _heart_beat();                     // 心跳包函数,   进入APP界面后每分钟执行一次

    void halt();                     // 停止窗帘
    void pull(double target_state);  // 拉起或拉下窗帘
    void pullup(uint32_t time_diff); // 拉起窗帘
    void pulldn(uint32_t time_diff); // 拉下窗帘
    void start_timer(uint32_t elapsed_time); // 开始计时
    void print_state();                      // 打印窗帘的状态
    uint32_t stop_timer();                   // 停止计时

    void pin_init();                 // 开发板引脚初始化
    void attach_all()                // 绑定所有回调函数
    {
        btn_up.attach(_btn_up);          // 绑定按钮回调函数
        btn_dn.attach(_btn_dn);
        btn_dbg.attach(_btn_dbg);
        btn_rst.attach(_btn_rst);
        btn_led.attach(_btn_led);
        sld_tg.attach(_sld_tg);
        Blinker.attachDataStorage(_data_storage);   // 绑定历史数据存储函数
        Blinker.attachHeartbeat(_heart_beat);       // 绑定心跳包函数
        Blinker.attachRTData(_rtData);              // 绑定实时数据函数
    }

#pragma endregion 按钮注册和函数声明


#pragma region 主程序区

    void setup()
    {
        Serial.begin(115200);             // 初始化串口, 波特率为115200
        BLINKER_DEBUG.stream(Serial);     // 启用串口输出调试信息
        // BLINKER_DEBUG.debugAll();      // 启用详细调试信息
        Blinker.begin(auth, ssid, pswd);  // 启动BLINKER, 连接WIFI
        pin_init();                       // 开发板引脚初始化
        attach_all();                     // 绑定所有回调函数
    }

    void loop()
    {
        if (timer != nullptr) { timer->update(); } // 计时器更新
        Blinker.run();
    }

#pragma endregion 主程序区


#pragma region 函数定义区

    void pin_init()  // 开发板引脚初始化
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

    void pull(double target_state)
    {
        double state_diff = (target_state - curtain_state);
        if (DEBUG)
        {
            BLINKER_LOG("================================== in function pull ==================================");
            BLINKER_LOG("state diff: ", state_diff);
            BLINKER_LOG("================================== in function pull ==================================");
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
            halt();
        }
    }

    void pullup(uint32_t time_diff = 0)
    {
        if (DEBUG)
        {
            BLINKER_LOG("================================== in function pullup ==================================");
            BLINKER_LOG("time_diff: ", time_diff);
            BLINKER_LOG("================================== in function pullup ==================================");
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

    void pulldn(uint32_t time_diff = 0)
    {

        if (DEBUG)
        {
            BLINKER_LOG("================================== in function pulldn ==================================");
            BLINKER_LOG("time_diff: ", time_diff);
            BLINKER_LOG("================================== in function pulldn ==================================");
        }
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
        sld_sta.print(int(curtain_state * 100));
    }

    void halt()
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
            BLINKER_LOG("================================== in function halt ==================================");
            BLINKER_LOG("time_diff: ", time_diff);
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
        sld_tg.print(curtain_state * 100);

        if (DEBUG)
        {
            BLINKER_LOG("================================== in function halt ==================================");
        }
    }

    void start_timer(uint32_t elapsed_time)
    {
        if (timer != nullptr)
            return;
        start_time = millis();
        timer = new TickTwo(halt, elapsed_time, 1, MILLIS);
        timer->start();
    }

    uint32_t stop_timer()
    {
        uint32_t time_diff = millis() - start_time;
        if (DEBUG)
        {
            BLINKER_LOG("================================== in function stop_timer ==================================");
            BLINKER_LOG("time_diff: ", time_diff);
            BLINKER_LOG("================================== in function stop_timer ==================================");
        }
        timer->stop();
        delete timer;
        timer = nullptr;
        return time_diff;
    }

    void _btn_up(const String &state)
    {
        if (state == BLINKER_CMD_BUTTON_TAP && motion_state == 'h')
        {
            pull(0);
        }
        else if (state == BLINKER_CMD_BUTTON_PRESS)
        {
            pullup();
        }
        else // BLINKER_CMD_BUTTON_PRESSUP
        {
            halt();
        }
    }

    void _btn_dn(const String &state)
    {
        if (state == BLINKER_CMD_BUTTON_TAP && motion_state == 'h')
        {
            pull(1);
        }
        else if (state == BLINKER_CMD_BUTTON_PRESS)
        {
            pulldn();
        }
        else // pressup
        {
            halt();
        }
    }

    void _btn_dbg(const String &state)
    {

        BLINKER_LOG("================================== in function debug ==================================");
        print_state();
        BLINKER_LOG("================================== in function debug ==================================");
    }

    void _btn_rst(const String &state)
    {

        if (DEBUG)
        {
            BLINKER_LOG("================================== in function reset ==================================");
            print_state();
            BLINKER_LOG("================================== in function reset ==================================");

            halt();
            motion_state = 'h';
            curtain_state = 1.0;

            if (DEBUG)
            {
                BLINKER_LOG("================================== in function reset ==================================");
                print_state();
                BLINKER_LOG("================================== in function reset ==================================");
            }
        }
    }

    void _btn_led(const String &state)
    {
        if (state == BLINKER_CMD_ON)
        {
            use_led = true;
            btn_led.print(BLINKER_CMD_ON);
        }
        else if (state == BLINKER_CMD_OFF)
        {
            use_led = false;
            digitalWrite(LED_BUILTIN, LOW);
            btn_led.print(BLINKER_CMD_OFF);
        }
    }

    void _sld_tg(int32_t data)
    {

        if (DEBUG)
        {
            BLINKER_LOG("================================== in function sld_tg ==================================");
            BLINKER_LOG("data: ", data);
            BLINKER_LOG("================================== in function sld_tg ==================================");
        }

        pull((static_cast<double>(data) / 100.0));
    }

    void print_state()
    {
        BLINKER_LOG("DEBUG_INFO:-----------------------------------------------------------");
        BLINKER_LOG("DEBUG_INFO:curtain_state: ", curtain_state);
        BLINKER_LOG("DEBUG_INFO:motion_state: ", motion_state);
        BLINKER_LOG("DEBUG_INFO:-----------------------------------------------------------");
    }

    void _data_storage() // 历史数据存储函数, 每分钟执行一次
    {
        BLINKER_LOG("================================== in function data_storage ==================================");
        BLINKER_LOG("curtain_state: ", curtain_state);
        BLINKER_LOG("================================== in function data_storage ==================================");

        Blinker.dataStorage("curtain_state", curtain_state);
    }

    void _rtData() // 实时数据上传函数, 每秒执行一次
    {
        BLINKER_LOG("rtData()");

        Blinker.sendRtData("num_sta", curtain_state * 100);
        Blinker.sendRtData("ran_sta", curtain_state * 100);
        Blinker.printRtData();
    }

    void _heart_beat() // 心跳包函数, 进入APP界面后每分钟执行一次, 适用于初始化APP界面
    {
        BLINKER_LOG("heart_beat()");

        if (use_led)
            btn_led.print("on");
        else
            btn_led.print("off");

        sld_tg.print(curtain_state * 100);
    }

#pragma endregion 函数定义区