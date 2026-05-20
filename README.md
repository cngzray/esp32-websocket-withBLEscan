这是一个ESP32使用websocket的方式与前端网页通信的示例
附加了BLE设备扫描功能，可以通过网页命令的方式让ESP32进行蓝牙BLE扫描，并把结果返回到网页界面

ESP32采用了大分区方案，以便上传更多库和所需开发代码

; 使用大Flash分区方案
board_build.partitions = huge_app.csv

<img width="665" height="824" alt="image" src="https://github.com/user-attachments/assets/9236d8d0-6087-4313-9c24-b02ca0cf3475" />
