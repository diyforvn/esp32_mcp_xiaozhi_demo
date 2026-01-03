
#include <Arduino.h>
#include <WiFi.h>
#include <WebSocketMCP.h>



const char* WIFI_SSID = "PHUCLOC";
const char* WIFI_PASS = "Phucloctho79";


const char* MCP_ENDPOINT = "wss://api.xiaozhi.me/mcp/?token=eyJhbGciOiJFUzI1NiIsInR5cCI6IkpXVCJ9.eyJ1c2VySWQiOjY4NTY0MCwiYWdlbnRJZCI6MTE0Mjc5NiwiZW5kcG9pbnRJZCI6ImFnZW50XzExNDI3OTYiLCJwdXJwb3NlIjoibWNwLWVuZHBvaW50IiwiaWF0IjoxNzY3MzY1MDczLCJleHAiOjE3OTg5MjI2NzN9.20IF8bD-6uqXgLu7BN1nTgaJuHINiUL3kvcM1Lumai2r-9AZudV9Di7LkeA2w16cvmrznnLvC_TR87QVRw4x0w";


#define DEBUG_SERIAL Serial
#define DEBUG_BAUD_RATE 115200


#define LED_PIN 2  


WebSocketMCP mcpClient;


#define MAX_INPUT_LENGTH 1024
char inputBuffer[MAX_INPUT_LENGTH];
int inputBufferIndex = 0;
bool newCommandAvailable = false;


bool wifiConnected = false;
bool mcpConnected = false;


void setupWifi();
void onMcpOutput(const String &message);
void onMcpError(const String &error);
void onMcpConnectionChange(bool connected);
void processSerialCommands();
void blinkLed(int times, int delayMs);
void registerMcpTools();
void printHelp();
void printStatus();


void printHelp() {
  DEBUG_SERIAL.println("Các lệnh có sẵn:");
  DEBUG_SERIAL.println("  help     - Hiển thị thông báo trợ giúp này");
  DEBUG_SERIAL.println("  status   - Hiển thị trạng thái kết nối hiện tại");
  DEBUG_SERIAL.println("  reconnect - Kết nối lại với máy chủ MCP");
  DEBUG_SERIAL.println("  tools    - Xem các công cụ đã đăng ký");
  DEBUG_SERIAL.println("  Mọi nội dung văn bản khác sẽ được gửi trực tiếp đến máy chủ MCP.");
}

void printStatus() {
  DEBUG_SERIAL.println("Status:");
  DEBUG_SERIAL.print("  WiFi: ");
  DEBUG_SERIAL.println(wifiConnected ? "Connected" : "Not connected");
  if (wifiConnected) {
    DEBUG_SERIAL.print("  IP: ");
    DEBUG_SERIAL.println(WiFi.localIP());
    DEBUG_SERIAL.print("  RSSI: ");
    DEBUG_SERIAL.println(WiFi.RSSI());
  }
  DEBUG_SERIAL.print("  MCPServer: ");
  DEBUG_SERIAL.println(mcpConnected ? "Connected" : "Not connected");
}

void setup() {
 
  DEBUG_SERIAL.begin(DEBUG_BAUD_RATE);
  DEBUG_SERIAL.println("\n\n[ESP32 MCP init...");
  
 
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  
  setupWifi();
  
 
  if (mcpClient.begin(MCP_ENDPOINT, onMcpConnectionChange)) {
    DEBUG_SERIAL.println("[ESP32 MCP Slave] Khởi tạo thành công. Đang cố gắng kết nối với máy chủ MCP....");
  } else {
    DEBUG_SERIAL.println("[ESP32 MCP Slave] Khởi tạo không thành công!");
  }
  
  
  DEBUG_SERIAL.println("Hướng dẫn sử dụng:");
  DEBUG_SERIAL.println("- Nhập lệnh vào cửa sổ dòng lệnh và nhấn Enter để gửi.");
  DEBUG_SERIAL.println("- Các thông báo nhận được từ máy chủ MCP sẽ được hiển thị trên giao diện điều khiển nối tiếp.");
  DEBUG_SERIAL.println("- Nhập \"help\" Xem các lệnh có sẵn");
  DEBUG_SERIAL.println();
}

void loop() {
 
  mcpClient.loop();
  
  processSerialCommands();
  
  if (!wifiConnected) {
    blinkLed(1, 100);
  } else if (!mcpConnected) {
    blinkLed(1, 500);
  }// else {
  //  digitalWrite(LED_PIN, HIGH);
  //}
}


void setupWifi() {
  DEBUG_SERIAL.print("[WiFi] đang kết nối ");
  DEBUG_SERIAL.println(WIFI_SSID);
  
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    DEBUG_SERIAL.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    DEBUG_SERIAL.println();
    DEBUG_SERIAL.println("[WiFi] kết nối thành công!");
    DEBUG_SERIAL.print("[WiFi] IP local: ");
    DEBUG_SERIAL.println(WiFi.localIP());
  } else {
    wifiConnected = false;
    DEBUG_SERIAL.println();
    DEBUG_SERIAL.println("[WiFi] Kết nối thất bại! Sẽ tiếp tục thử lại...");
  }
}


void onMcpOutput(const String &message) {
  DEBUG_SERIAL.print("[MCP Out] ");
  DEBUG_SERIAL.println(message);
}


void onMcpError(const String &error) {
  DEBUG_SERIAL.print("[MCPError] ");
  DEBUG_SERIAL.println(error);
}


void registerMcpTools() {
  DEBUG_SERIAL.println("[MCP] Đăng ký...");
  
 
  mcpClient.registerTool(
    "led_blink",  
    "LED-control", 
    "{\"properties\":{\"state\":{\"title\":\"LEDStatus\",\"type\":\"string\",\"enum\":[\"on\",\"off\",\"blink\"]}},\"required\":[\"state\"],\"title\":\"ledControlArguments\",\"type\":\"object\"}",  
    [](const String& args) {     
      DEBUG_SERIAL.println("[Công cụ] Điều khiển đèn LED: " + args);
      DynamicJsonDocument doc(256);
      DeserializationError error = deserializeJson(doc, args);
      
      if (error) {
       
        WebSocketMCP::ToolResponse response("{\"success\":false,\"error\":\"Định dạng tham số không hợp lệ\"}", true);
        return response;
      }
      
      String state = doc["state"].as<String>();
      DEBUG_SERIAL.println("[Công cụ] Điều khiển đèn LED: " + state);
      
      
      if (state == "on") {
        digitalWrite(LED_PIN, HIGH);
      } else if (state == "off") {
        digitalWrite(LED_PIN, LOW);
      } else if (state == "blink") {
        for (int i = 0; i < 5; i++) {
          digitalWrite(LED_PIN, HIGH);
          delay(200);
          digitalWrite(LED_PIN, LOW);
          delay(200);
        }
      }
      
      
      String resultJson = "{\"success\":true,\"state\":\"" + state + "\"}";
      return WebSocketMCP::ToolResponse(resultJson);
    }
  );
  DEBUG_SERIAL.println("[MCP] LED-Control đã được đăng ký.");
  
  
  mcpClient.registerTool(
    "system-info",
    "ESP32-info",
    "{\"properties\":{},\"title\":\"systemInfoArguments\",\"type\":\"object\"}",
    [](const String& args) {
      DEBUG_SERIAL.println("[ESP32-info: " + args);
      
      String chipModel = ESP.getChipModel();
      uint32_t chipId = ESP.getEfuseMac() & 0xFFFFFFFF;
      uint32_t flashSize = ESP.getFlashChipSize() / 1024;
      uint32_t freeHeap = ESP.getFreeHeap() / 1024;
      
      
      String resultJson = "{\"success\":true,\"model\":\"" + chipModel + "\",\"chipId\":\"" + String(chipId, HEX) + 
                         "\",\"flashSize\":" + String(flashSize) + ",\"freeHeap\":" + String(freeHeap) + 
                         ",\"wifiStatus\":\"" + (WiFi.status() == WL_CONNECTED ? "connected" : "disconnected") + 
                         "\",\"ipAddress\":\"" + WiFi.localIP().toString() + "\"}";
      
      return WebSocketMCP::ToolResponse(resultJson);
    }
  );
  DEBUG_SERIAL.println("[MCP] ESP32-info đã đăng ký");
  
 
  mcpClient.registerTool(
    "calculator",
    "Tính toán",
    "{\"properties\":{\"expression\":{\"title\":\"biểu lộ\",\"type\":\"string\"}},\"required\":[\"expression\"],\"title\":\"calculatorArguments\",\"type\":\"object\"}",
    [](const String& args) {
       DEBUG_SERIAL.println("[tính toán: " + args);
      DynamicJsonDocument doc(256);
      deserializeJson(doc, args);
      
      String expr = doc["expression"].as<String>();
      DEBUG_SERIAL.println("[Công cụ] Tính toán: " + expr);
      
      
      int result = 0;
      if (expr.indexOf("+") > 0) {
        int plusPos = expr.indexOf("+");
        int a = expr.substring(0, plusPos).toInt();
        int b = expr.substring(plusPos + 1).toInt();
        result = a + b;
      } else if (expr.indexOf("-") > 0) {
        int minusPos = expr.indexOf("-");
        int a = expr.substring(0, minusPos).toInt();
        int b = expr.substring(minusPos + 1).toInt();
        result = a - b;
      }
      
      String resultJson = "{\"success\":true,\"expression\":\"" + expr + "\",\"result\":" + String(result) + "}";
      return WebSocketMCP::ToolResponse(resultJson);
    }
  );
  DEBUG_SERIAL.println("[MCP] Công cụ tính toán đã được đăng ký");
  
  DEBUG_SERIAL.println("[MCP] Đăng ký công cụ hoàn tất, tổng cộng" + String(mcpClient.getToolCount()) + "công cụ");
}


void onMcpConnectionChange(bool connected) {
  mcpConnected = connected;
  if (connected) {
    DEBUG_SERIAL.println("[MCP] Đã kết nối với máy chủ MCP");
    // 连接成功后注册工具
    registerMcpTools();
  } else {
    DEBUG_SERIAL.println("[MCP] Ngắt kết nối khỏi máy chủ MCP");
  }
}


void processSerialCommands() {
  while (DEBUG_SERIAL.available() > 0) {
    char inChar = (char)DEBUG_SERIAL.read();
    
    if (inChar == '\n' || inChar == '\r') {
      if (inputBufferIndex > 0) {
        inputBuffer[inputBufferIndex] = '\0';
        
        String command = String(inputBuffer);
        command.trim();
        
        if (command.length() > 0) {
          if (command == "help") {
            printHelp();
          } else if (command == "status") {
            printStatus();
          } else if (command == "reconnect") {
            DEBUG_SERIAL.println("Đang kết nối lại...");
            mcpClient.disconnect();
          } else if (command == "tools") {
            // 显示已注册工具
            DEBUG_SERIAL.println("Số lượng công cụ đã đăng ký: " + String(mcpClient.getToolCount()));
          } else {
            if (mcpClient.isConnected()) {
              mcpClient.sendMessage(command);
              DEBUG_SERIAL.println("[gửi] " + command);
            } else {
              DEBUG_SERIAL.println("Không thể kết nối với máy chủ MCP, không thể gửi lệnh.");
            }
          }
        }
        inputBufferIndex = 0;
      }
    } 
    
    else if (inChar == '\b' || inChar == 127) {
      if (inputBufferIndex > 0) {
        inputBufferIndex--;
        DEBUG_SERIAL.print("\b \b"); 
      }
    }
    
    else if (inputBufferIndex < MAX_INPUT_LENGTH - 1) {
      inputBuffer[inputBufferIndex++] = inChar;
      DEBUG_SERIAL.print(inChar); 
    }
  }
}




void blinkLed(int times, int delayMs) {
  static int blinkCount = 0;
  static unsigned long lastBlinkTime = 0;
  static bool ledState = false;
  static int lastTimes = 0;

  if (times == 0) {
    digitalWrite(LED_PIN, LOW);
    blinkCount = 0;
    lastTimes = 0;
    return;
  }
  if (lastTimes != times) {
    blinkCount = 0;
    lastTimes = times;
    ledState = false;
    lastBlinkTime = millis();
  }
  unsigned long now = millis();
  if (blinkCount < times * 2) {
    if (now - lastBlinkTime > delayMs) {
      lastBlinkTime = now;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      blinkCount++;
    }
  } else {
    digitalWrite(LED_PIN, LOW);
    blinkCount = 0;
    lastTimes = 0;
  }
}