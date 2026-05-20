#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

const char* ssid = "ssid";
const char* password = "password";

// 创建WebSocket服务器，端口81
WebSocketsServer webSocket = WebSocketsServer(81);

// BLE扫描对象
BLEScan* pBLEScan;

// BLE定时扫描相关
unsigned long lastScanTime = 0;
const unsigned long scanInterval = 10000;  // 10秒

// BLE设备列表（用于汇聚扫描结果）
String bleDeviceList = "";
int bleDeviceCount = 0;

// WebSocket事件回调函数声明
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);

// 根据厂商ID获取厂商名称
String getManufacturerName(String manufacturerId) {
    if (manufacturerId.equalsIgnoreCase("004C")) return "Apple";
    if (manufacturerId.equalsIgnoreCase("0006")) return "Microsoft";
    if (manufacturerId.equalsIgnoreCase("02E0")) return "Huawei";
    if (manufacturerId.equalsIgnoreCase("0944")) return "Xiaomi";
    if (manufacturerId.equalsIgnoreCase("0059")) return "Samsung";
    if (manufacturerId.equalsIgnoreCase("00E0")) return "Google";
    if (manufacturerId.equalsIgnoreCase("000A")) return "Sony";
    if (manufacturerId.equalsIgnoreCase("0010")) return "LG";
    if (manufacturerId.equalsIgnoreCase("000E")) return "HTC";
    if (manufacturerId.equalsIgnoreCase("000F")) return "Nokia";
    if (manufacturerId.equalsIgnoreCase("000B")) return "Motorola";
    if (manufacturerId.equalsIgnoreCase("02E9")) return "Oppo";
    if (manufacturerId.equalsIgnoreCase("0312")) return "Vivo";
    if (manufacturerId.equalsIgnoreCase("0534")) return "OnePlus";
    return "Unknown";
}

// BLE扫描回调类
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        // 获取设备名称
        String deviceName = "Unknown";
        if (advertisedDevice.haveName()) {
            deviceName = advertisedDevice.getName().c_str();
            if (deviceName.length() == 0) {
                deviceName = "EmptyName";
            }
        }
        
        // 获取厂商ID
        String manufacturerId = "N/A";
        String manufacturerName = "Unknown";
        if (advertisedDevice.haveManufacturerData()) {
            std::string manufData = advertisedDevice.getManufacturerData();
            if (manufData.length() >= 2) {
                uint16_t manufId = (uint16_t)(manufData[1] << 8) | manufData[0];
                // 确保厂商ID是4位十六进制字符串，前面补零
                char manufIdStr[5];
                sprintf(manufIdStr, "%04X", manufId);
                manufacturerId = String(manufIdStr);
                // 根据厂商ID获取厂商名称
                manufacturerName = getManufacturerName(manufacturerId);
            }
        }
        
        // 将设备信息添加到列表（JSON数组元素格式）
        String deviceInfo = "{\"name\":\"" + deviceName + "\",\"address\":\"" + advertisedDevice.getAddress().toString().c_str() + "\",\"rssi\":" + String(advertisedDevice.getRSSI()) + ",\"manufacturerId\":\"" + manufacturerId + "\",\"manufacturer\":\"" + manufacturerName + "\"}";
        
        // 添加到设备列表（如果不是第一个，前面加逗号）
        if (bleDeviceList.length() > 0) {
            bleDeviceList += ",";
        }
        bleDeviceList += deviceInfo;
        bleDeviceCount++;
        
        // 串口输出
        Serial.printf("BLE设备发现: %s, 地址: %s, RSSI: %d dBm, 厂商ID: %s, 厂商: %s\n", 
                      deviceName.c_str(),
                      advertisedDevice.getAddress().toString().c_str(),
                      advertisedDevice.getRSSI(),
                      manufacturerId.c_str(),
                      manufacturerName.c_str());
    }
};

// 扫描BLE设备（广播模式）
void scanBLE() {
    Serial.println("开始扫描BLE设备...");
    webSocket.broadcastTXT("开始扫描BLE设备...");
    
    // 清空设备列表和计数器
    bleDeviceList = "";
    bleDeviceCount = 0;
    
    // 开始扫描，扫描时间3秒（缩短扫描时间以适应10秒周期）
    pBLEScan->start(3, false);
    
    // 扫描完成后，汇聚所有结果为一个JSON数组
    String resultJson = "{\"devices\":[" + bleDeviceList + "],\"count\":" + String(bleDeviceCount) + "}";
    
    Serial.println("BLE扫描完成");
    Serial.printf("扫描到 %d 个BLE设备\n", bleDeviceCount);
    Serial.println("扫描结果: " + resultJson);
    
    // 发送完整的JSON到WebSocket
    webSocket.broadcastTXT(resultJson);
}

void setup() {
  // 初始化串口，波特率115200
  Serial.begin(115200);
  
  // 等待串口就绪
  delay(10);
  
  Serial.println();
  Serial.println("==============================");
  Serial.printf("正在连接 WiFi: %s\n", ssid);
  
  // 连接WiFi
  WiFi.begin(ssid, password);
  
  // 等待连接
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println();
  Serial.println("WiFi 连接成功!");
  Serial.println("==============================");
  
  // 输出网络信息
  Serial.println("\n网络信息:");
  Serial.printf("WiFi SSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("WiFi 信号强度: %d dBm\n", WiFi.RSSI());
  
  // 输出IP信息
  Serial.println("\nIP 地址信息:");
  Serial.printf("本地IP地址: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("子网掩码: %s\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("网关地址: %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("DNS服务器: %s\n", WiFi.dnsIP().toString().c_str());
  
  Serial.println("\n==============================");
  
  // WebSocket事件处理函数
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  
  Serial.println("WebSocket服务器已启动，端口: 81");
  Serial.println("WebSocket连接地址: ws://" + WiFi.localIP().toString() + ":81");
  
  // 初始化BLE
  BLEDevice::init("ESP32_BLE_Scanner");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);  // 主动扫描
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
  
  Serial.println("BLE扫描器已初始化");
}

// WebSocket事件回调函数
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_CONNECTED: {
      Serial.printf("WebSocket客户端连接: %u\n", num);
      // 发送欢迎消息
      webSocket.sendTXT(num, "欢迎连接到ESP32 WebSocket服务器!");
      break;
    }
    case WStype_DISCONNECTED: {
      Serial.printf("WebSocket客户端断开: %u\n", num);
      break;
    }
    case WStype_TEXT: {
      // 接收到文本消息
      Serial.printf("收到客户端 %u 的消息: ", num);
      for(size_t i=0; i<length; i++) {
        Serial.print((char)payload[i]);
      }
      Serial.println();
      
      // 解析命令
      String cmd = String((char*)payload);
      
      if (cmd == "scan" || cmd == "ble scan" || cmd == "扫描BLE") {
        // 执行BLE扫描
        scanBLE();
      } else {
        // 响应消息（回显）
        String response = "ESP32收到: " + cmd;
        webSocket.sendTXT(num, response);
        
        // 广播消息给所有客户端
        String broadcastMsg = "客户端 " + String(num) + " 说: " + cmd;
        webSocket.broadcastTXT(broadcastMsg);
      }
      break;
    }
    case WStype_BIN:
    case WStype_ERROR:
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
    case WStype_PING:
    case WStype_PONG:
      break;
  }
}

void loop() {
  // 处理WebSocket事件
  webSocket.loop();
  
  // 每隔10秒自动扫描一次BLE
  /*---
  unsigned long currentTime = millis();
  if (currentTime - lastScanTime >= scanInterval) {
    lastScanTime = currentTime;
    scanBLE();
  }
  */
 
  delay(10);
}