# ESP32 WebSocket BLE Scanner
## 项目简介
该项目基于ESP32实现**WiFi + WebSocket服务端 + BLE蓝牙广播扫描**一体化功能：
1. ESP32连接WiFi后启动WebSocket服务（81端口）；
2. 持续扫描周边BLE广播设备，解析设备名称、MAC地址、RSSI信号、厂商ID与厂商名称；
3. 前端HTML页面通过WebSocket实时连接ESP32，下发扫描指令并可视化展示扫描到的蓝牙设备列表；
4. 支持手动触发扫描，可扩展定时自动扫描，数据以标准JSON格式下发前端，界面自动渲染设备卡片。

适用场景：室内蓝牙设备探测、蓝牙信标监控、周边手机/手环蓝牙设备巡检、物联网BLE广播采集。

<img width="665" height="824" alt="image" src="https://github.com/user-attachments/assets/9236d8d0-6087-4313-9c24-b02ca0cf3475" />

## 功能特性
### 硬件端（ESP32）
- WiFi STA模式联网，打印完整网络信息（IP、网关、DNS、WiFi信号）；
- WebSocket服务端（端口81），支持多客户端同时连接、消息广播；
- BLE主动扫描，扫描时长3s，可配置扫描间隔；
- 解析蓝牙广播厂商数据，内置主流手机厂商ID映射（苹果、华为、小米、三星、OPPO、vivo等）；
- 扫描结果封装标准JSON数组，一键广播给所有WebSocket客户端；
- 串口完整打印扫描日志，方便调试；
- 支持前端下发指令触发扫描：`scan` / `ble scan` / `扫描BLE`；
- 收到普通文本消息自动回显并广播给全部在线客户端；
- 预留10s自动定时扫描逻辑（注释放开即可启用）。

### Web前端（纯HTML/JS，无需服务器）
- WebSocket连接控制面板，自定义ESP32 IP与端口；
- 连接状态实时展示（已连接/断开/异常）；
- 通用消息收发窗口，支持回车快捷发送；
- 独立BLE设备可视化面板，自动渲染扫描结果；
- 设备卡片展示：设备名、MAC地址、RSSI信号强度、厂商、厂商ID；
- 无设备/未扫描状态友好提示；
- 区分系统消息、服务端消息、本地发送消息，样式区分；
- 自适应简约UI，PC/手机浏览器均可打开。

## 硬件需求
- ESP32开发板（任意型号，内置BLE与WiFi）
- USB数据线（供电+程序烧录）
- 2.4G WiFi（不支持5G WiFi）

## 软件环境
### Arduino IDE
1. 安装ESP32开发板支持包；
   ; 使用大Flash分区方案
  board_build.partitions = huge_app.csv
3. 安装依赖库：
   - `WebSocketsServer`（WebSocket服务）
   - ESP32自带BLE库（无需额外安装）

## 项目文件结构
```
esp32-websocket-withBLEscan/
├── src/
│   └── main.cpp       // ESP32 Arduino主程序
└── web/
    └── index.html     // WebSocket前端可视化页面
```

## 快速部署教程
### 1. ESP32程序烧录
1. 使用Arduino IDE打开`src/main.cpp`；
2. 修改WiFi账号密码：
   ```cpp
   const char* ssid = "你的WiFi名称";
   const char* password = "你的WiFi密码";
   ```
3. （可选）开启自动定时扫描：
   取消`loop()`中扫描代码注释，每10秒自动执行一次BLE扫描；
4. 选择对应ESP32开发板与串口，编译上传；
5. 打开串口监视器（波特率115200），等待WiFi连接成功，记录打印出的**本地IP地址**。

### 2. Web前端使用
1. 将`web/index.html`保存到本地；
2. 直接用浏览器（Chrome/Edge/Firefox）打开该文件，无需本地服务器；
3. 在页面IP输入框填入串口打印的ESP32本地IP，端口默认81；
4. 点击「连接」建立WebSocket通道；
5. 在消息输入框发送 `scan` 触发蓝牙扫描，下方BLE设备列表会自动刷新结果。

## 通信协议说明
### 1. 前端下发扫描指令
发送任意一条文本即可触发扫描：
- `scan`
- `ble scan`
- `扫描BLE`

### 2. ESP32下发BLE扫描结果JSON格式
```json
{
  "devices": [
    {
      "name": "Xiaomi Phone",
      "address": "XX:XX:XX:XX:XX:XX",
      "rssi": -62,
      "manufacturerId": "0944",
      "manufacturer": "Xiaomi"
    }
  ],
  "count": 1
}
```
字段说明：
- `name`：蓝牙广播名称，无名称显示`EmptyName`，未知显示`Unknown`
- `address`：蓝牙MAC地址
- `rssi`：信号强度（dBm，数值越大信号越好）
- `manufacturerId`：4位十六进制厂商ID
- `manufacturer`：解析后的厂商名称，无厂商数据显示`Unknown`

### 3. 普通文本消息交互
前端发送任意非扫描指令文本，ESP32会：
1. 单播回复 `ESP32收到: xxx`；
2. 广播给所有客户端 `客户端 0 说: xxx`。

## 关键参数配置说明（main.cpp）
```cpp
const unsigned long scanInterval = 10000; // 自动扫描间隔：10000ms=10s
pBLEScan->start(3, false); // 单次BLE扫描时长3秒
pBLEScan->setActiveScan(true); // 主动扫描，获取更多广播数据
pBLEScan->setInterval(100);
pBLEScan->setWindow(99);
```
可根据需求调整扫描时长、扫描间隔、扫描窗口参数。

## 支持厂商列表（厂商ID映射）
| 厂商ID | 品牌 |
|--------|------|
| 004C | Apple |
| 0006 | Microsoft |
| 02E0 | Huawei |
| 0944 | Xiaomi |
| 0059 | Samsung |
| 00E0 | Google |
| 000A | Sony |
| 0010 | LG |
| 000E | HTC |
| 000F | Nokia |
| 000B | Motorola |
| 02E9 | Oppo |
| 0312 | Vivo |
| 0534 | OnePlus |
无匹配厂商ID统一返回`Unknown`，可自行扩展`getManufacturerName`函数添加更多厂商。

