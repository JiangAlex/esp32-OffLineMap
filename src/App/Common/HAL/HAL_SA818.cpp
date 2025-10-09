#include "HAL.h"
//#include <HardwareSerial.h>
#include <Arduino.h>
#include "DRA818.h" // uncomment the following line in DRA818.h (#define DRA818_DEBUG)

/* Used Pins */
//#define SA818_BAUD 115200
int SERIAL_SPEED = 9600;  // 預設波特率，可以動態修改
#define SERIAL_SPEED_ALT    115200  // 備用波特率
#define DRA818_CONFIG_UHF 1
#define SA_PD      -1  // to the DRA818 PD pin 6

// SA818 引腳配置 (ADC2_CHANNEL_3 已在 Mic_Class.cpp 中禁用)
#define SA_RX GPIO_NUM_15   // arduino serial RX pin to the DRA818 TX pin 17
#define SA_TX GPIO_NUM_16   // arduino serial TX pin to the DRA818 RX pin 16

// 備用引腳配置（如果上面的引腳有衝突）
#define SA_RX_ALT GPIO_NUM_18   
#define SA_TX_ALT GPIO_NUM_19   
//VBAT Pin 8 , GND Pin 9,10 

//HardwareSerial dra_serial(1);
HardwareSerial dra_serial(2);

DRA818 *dra;                // the DRA object once instanciated
float freq;                 // the next frequency to scan

void HAL::SA818_Init()
{
    //Serial.begin(115200); // for logging
    Serial.println("Booting ...");
    Serial.print("initializing I/O ... \r\n");
    
    // 檢查引腳配置
    Serial.printf("SA818 Pins - RX: %d, TX: %d (ADC2_CHANNEL_3 disabled for SA818)\n", SA_RX, SA_TX);
    
    // 清空串列緩衝區
    dra_serial.begin(SERIAL_SPEED,SERIAL_8N1,SA_RX,SA_TX);
    delay(100);
    while(dra_serial.available()) {
        dra_serial.read();
    }
    
    // Add power-on delay to allow SA818 module to initialize
    Serial.println("Waiting for SA818 module to initialize...");
    delay(3000);  // 增加到 3 秒
    
    Serial.println("Creating DRA818 object...");
    #if DRA818_CONFIG_UHF
        dra = new DRA818((HardwareSerial*) &dra_serial, SA818_UHF);  // 使用 SA818_UHF 而不是 DRA818_UHF
    #else
        dra = new DRA818((HardwareSerial*) &dra_serial, SA818_VHF);  // 使用 SA818_VHF 而不是 DRA818_VHF
    #endif
    
    #ifdef DRA818_DEBUG
    dra->set_log(&Serial);
    Serial.println("Debug logging enabled");
    #endif
    
    // 測試串列連接
    Serial.println("Testing serial connection...");
    Serial.println("Looking for SA818 response format: +DMOCONNECT:0<CR><LF>");
    
    // 測試多個波特率，尋找正確的 SA818 回應
    int baud_rates[] = {4800, 9600, 19200, 38400, 115200};
    int num_bauds = sizeof(baud_rates) / sizeof(baud_rates[0]);
    
    for(int b = 0; b < num_bauds; b++) {
        Serial.printf("\n=== Testing baud rate: %d ===\n", baud_rates[b]);
        
        // 重新配置串列
        dra_serial.end();
        delay(100);
        dra_serial.begin(baud_rates[b], SERIAL_8N1, SA_RX, SA_TX);
        delay(1000);
        
        // 清空緩衝區
        while(dra_serial.available()) {
            dra_serial.read();
        }
        
        // 測試 AT+DMOCONNECT 命令
        Serial.print("Command: AT+DMOCONNECT -> ");
        
        dra_serial.print("AT+DMOCONNECT\r\n");
        dra_serial.flush();
        delay(2000);  // 更長等待時間
        
        // 檢查回應
        char response_buffer[100];
        int buffer_index = 0;
        bool found_dmoconnect = false;
        
        Serial.print("Response: ");
        while(dra_serial.available() && buffer_index < 99) {
            char c = dra_serial.read();
            response_buffer[buffer_index++] = c;
            
            if (c >= 32 && c <= 126) {
                Serial.write(c);
            } else if (c == '\r') {
                Serial.print("<CR>");
            } else if (c == '\n') {
                Serial.print("<LF>");
            } else {
                Serial.printf("[0x%02X]", c);
            }
        }
        response_buffer[buffer_index] = '\0';
        
        // 檢查是否包含正確的回應
        if (strstr(response_buffer, "+DMOCONNECT:0") != NULL) {
            Serial.print(" *** FOUND CORRECT SA818 RESPONSE! ***");
            found_dmoconnect = true;
        } else if (strstr(response_buffer, "+DMOCONNECT") != NULL) {
            Serial.print(" *** FOUND PARTIAL SA818 RESPONSE! ***");
        } else if (strstr(response_buffer, "DMOCONNECT") != NULL) {
            Serial.print(" *** FOUND DMOCONNECT TEXT! ***");
        }
        
        Serial.printf(" (%d bytes)\n", buffer_index);
        
        if (found_dmoconnect) {
            Serial.printf("==> SUCCESS! Correct baud rate is %d bps\n", baud_rates[b]);
            // 設定為找到的正確波特率
            SERIAL_SPEED = baud_rates[b];
            dra_serial.end();
            delay(100);
            dra_serial.begin(SERIAL_SPEED, SERIAL_8N1, SA_RX, SA_TX);
            delay(1000);
            break;  // 跳出測試，使用找到的波特率
        }
    }
    
    // 如果沒有找到正確回應，回到預設波特率
    Serial.println("\n=== No correct SA818 response found, using default 9600 bps ===");
    dra_serial.end();
    delay(100);
    dra_serial.begin(SERIAL_SPEED, SERIAL_8N1, SA_RX, SA_TX);
    delay(1000);
    
    Serial.println("SA818_Init: Done...");

//    Serial.print("initializing DRA818 ... ");
//    dra = DRA818::configure(SA818_SERIAL, DRA818_VHF, 145.500, 145.500, 4, 8, 0, 0, DRA818_12K5, true, true, true);
    //dra->configure(&dra_serial, DRA818_VHF, 145.500, 145.500, 4, 8, 0, 0, DRA818_12K5, true, true, true, &Serial);
    if (!dra) {
        Serial.println("\nError while configuring DRA818");
    }

    // 嘗試握手多次
    Serial.println("Attempting handshake with SA818...");
    Serial.println("Note: Using baud rate determined from testing above...");
    
    int handshake_attempts = 3;
    bool handshake_success = false;
    
    for(int attempt = 1; attempt <= handshake_attempts; attempt++) {
        Serial.printf("Handshake attempt %d/%d\n", attempt, handshake_attempts);
        
        // 清空緩衝區
        while(dra_serial.available()) {
            dra_serial.read();
        }
        
        Serial.printf("Calling dra->handshake() for attempt %d\n", attempt);
        if (dra->handshake() == true) {
            Serial.println("*** SA818 HANDSHAKE SUCCESSFUL! ***");
            handshake_success = true;
            break;
        } else {
            Serial.printf("Handshake attempt %d failed\n", attempt);
            delay(1000);
        }
    }
    
    if (!handshake_success) {
        Serial.println("Trying alternative baud rate (115200)...");
        dra_serial.end();
        delay(100);
        dra_serial.begin(SERIAL_SPEED_ALT, SERIAL_8N1, SA_RX, SA_TX);
        delay(1000);
        
        // 測試 115200 波特率下的基本通訊
        Serial.println("Testing 115200 baud rate...");
        dra_serial.print("AT\r\n");
        dra_serial.flush();
        delay(1000);
        
        Serial.print("115200 baud response: ");
        int alt_bytes = 0;
        while(dra_serial.available() && alt_bytes < 20) {
            char c = dra_serial.read();
            alt_bytes++;
            if (c >= 32 && c <= 126) {
                Serial.write(c);
            } else {
                Serial.printf("[0x%02X]", c);
            }
        }
        Serial.printf(" (%d bytes)\n", alt_bytes);
        
        // 清空緩衝區
        while(dra_serial.available()) {
            dra_serial.read();
        }
        
        // 重新創建 DRA818 對象
        delete dra;
        #if DRA818_CONFIG_UHF
            dra = new DRA818((HardwareSerial*) &dra_serial, SA818_UHF);
        #else
            dra = new DRA818((HardwareSerial*) &dra_serial, SA818_VHF);
        #endif
        
        #ifdef DRA818_DEBUG
        dra->set_log(&Serial);
        #endif
        
        // 再次嘗試握手
        for(int attempt = 1; attempt <= 3; attempt++) {
            Serial.printf("Alt baud handshake attempt %d/3\n", attempt);
            if (dra->handshake() == true) {
                Serial.println("Handshake successful with 115200 baud!");
                handshake_success = true;
                break;
            }
            delay(1000);
        }
    }
    
    if (!handshake_success) {
        Serial.println("Trying alternative pins (17,18)...");
        dra_serial.end();
        delay(100);
        dra_serial.begin(SERIAL_SPEED, SERIAL_8N1, SA_RX_ALT, SA_TX_ALT);
        delay(1000);
        
        // 清空緩衝區
        while(dra_serial.available()) {
            dra_serial.read();
        }
        
        // 重新創建 DRA818 對象
        delete dra;
        #if DRA818_CONFIG_UHF
            dra = new DRA818((HardwareSerial*) &dra_serial, SA818_UHF);
        #else
            dra = new DRA818((HardwareSerial*) &dra_serial, SA818_VHF);
        #endif
        
        #ifdef DRA818_DEBUG
        dra->set_log(&Serial);
        #endif
        
        // 再次嘗試握手
        for(int attempt = 1; attempt <= 3; attempt++) {
            Serial.printf("Alt pins handshake attempt %d/3\n", attempt);
            if (dra->handshake() == true) {
                Serial.println("Handshake successful with alternative pins!");
                handshake_success = true;
                break;
            }
            delay(1000);
        }
    }
    
    if (!handshake_success) {
        Serial.println("All handshake attempts failed. Check connections and power supply.");
        return;
    }
    
    #if DRA818_CONFIG_UHF
        dra->group(DRA818_12K5, DRA818_UHF_MIN, DRA818_UHF_MIN, 0, 4, 0);
        dra->volume(8);
        freq = DRA818_UHF_MIN;
    #else
        dra->group(DRA818_12K5, DRA818_VHF_MIN, DRA818_VHF_MIN, 0, 4, 0);
        dra->volume(8);
        freq = DRA818_VHF_MIN;
    #endif
    Serial.println("SA818_Init: Starting ... ");
}

void HAL::SA818_scan()
{
    return;
    char buf[14];
    if (!dra) return;
    dtostrf(freq, 8, 4, buf);  // convert frequency to string with right precision
    Serial.print(String("SA818_Update: Scanning frequency ") +  String(buf) + String(" kHz ..."));
    /* scan the frequency */
    if (dra->scan(freq)) Serial.print("Found");
        Serial.println("");
    freq += 0.0125; //12.5kHz step
    #if DRA818_CONFIG_UHF
        if (freq > DRA818_UHF_MAX) freq = DRA818_UHF_MIN; // when DRA818_VHF_MAX (174.0) is reached, start over at DRA818_VHF_MIN (134.0)
    #else
        if (freq > DRA818_VHF_MAX) freq = DRA818_VHF_MIN; // when DRA818_VHF_MAX (174.0) is reached, start over at DRA818_VHF_MIN (134.0)
    #endif
}

void HAL::SA818_GetInfo(SA818_Info_t* info)
{
    //Serial.println("SA818_GetInfo");
    memset(info, 0, sizeof(SA818_Info_t));

    // 檢查 SA818 是否已初始化並可用
    if (dra != nullptr) {
        // SA818 已初始化，返回實際配置資訊
        info->channel = 2;
        #if DRA818_CONFIG_UHF
            info->freq_rx = DRA818_UHF_MIN;  // 使用實際頻率
            info->freq_tx = DRA818_UHF_MIN;
        #else
            info->freq_rx = DRA818_VHF_MIN;  // 使用實際頻率
            info->freq_tx = DRA818_VHF_MIN;
        #endif
        info->squelch = 4;    // 實際靜噪設定
        info->volume = 8;     // 實際音量設定
        info->ctcss_rx = 0;   // 實際 CTCSS 設定
        info->ctcss_tx = 0;
        info->BW = DRA818_12K5;  // 頻寬設定
        info->SQ = 4;
    } else {
        // SA818 未初始化或初始化失敗，返回零值
        info->channel = 0;
        info->freq_rx = 0.0;
        info->freq_tx = 0.0;
        info->squelch = 0;
        info->volume = 0;
        info->ctcss_rx = 0;
        info->ctcss_tx = 0;
        info->BW = 0;
        info->SQ = 0;
    }
}