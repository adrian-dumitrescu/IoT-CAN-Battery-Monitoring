/*********************************************

   Copyright (c) [2020] [Adrian Dumitrescu]
   #Orion BMS Firmware
   Complete project details coming soon...

**********************************************/

#include "FS.h" //this needs to be first, or it all crashes and burns...
#include <ArduinoJson.h>
#include <WiFiManager.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

//#include <ESP8266WiFi.h>
//#include <ESP8266WiFiAP.h>
//#include <ESP8266WiFiGeneric.h>
//#include <ESP8266WiFiMulti.h>
//#include <ESP8266WiFiScan.h>
//#include <ESP8266WiFiSTA.h>
//#include <ESP8266WiFiType.h>
//#include <WiFiClient.h>
//#include <WiFiClientSecure.h>
//#include <WiFiClientSecureAxTLS.h>
//#include <WiFiClientSecureBearSSL.h>
//#include <WiFiServer.h>
//#include <WiFiServerSecure.h>
//#include <WiFiServerSecureAxTLS.h>
//#include <WiFiServerSecureBearSSL.h>
//#include <WiFiUdp.h>
//#include <ESP8266WiFi.h>

/*************************************************************************************
                                  TFT Display Config
*************************************************************************************/
#include <Adafruit_GFX.h>    // Core graphics library
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
//#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
//#include "TFT_Prime.h"
#define TFT_CS         0
#define TFT_RST        -1
#define TFT_DC         15

// For 1.44" and 1.8" TFT with ST7735 use:
//Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// For 1.3", 1.54", and 2.0" TFT with ST7789:
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

int CULOARE[] = {0x06BF, 0x0579, 0x0370, 0x022A, 0x01A8, 0x1125};

/* color definitions
  const uint16_t  Display_Color_Black        = 0x0000;
  const uint16_t  Display_Color_Blue         = 0x001F;
  const uint16_t  Display_Color_Red          = 0xF800;
  const uint16_t  Display_Color_Green        = 0x07E0;
  const uint16_t  Display_Color_Cyan         = 0x07FF;
  const uint16_t  Display_Color_Magenta      = 0xF81F;
  const uint16_t  Display_Color_Yellow       = 0xFFE0;
  const uint16_t  Display_Color_White        = 0xFFFF;

  /* The colors we actually want to use
  uint16_t        Display_Text_Color         = Display_Color_Black;
  uint16_t        Display_Backround_Color    = Display_Color_Blue;
*/

int loading_cycle = 0;
int TFT_SOC;
int ROUND_TFT_SOC;
int OLD_TFT_SOC = 0;
int TFT_WiFi_Strength;

const unsigned long TFT_LoadingBar_Event = 150;
const unsigned long TFT_Battery_Event = 1100;
const unsigned long TFT_WiFi_Event = 2000;

unsigned long previousTimeLoading;
unsigned long previousTimeBattery;
unsigned long previousTimeWiFi;


//***************************************************************//


/*************************************************************************************
                                  ThingsBoard Config
*************************************************************************************/

//ThingsBoard tb(wifiClient);
//WiFiEspClient espClient;

#include <ThingsBoard.h>
#include <PubSubClient.h>


WiFiClient wifiClient;
ThingsBoard tb(wifiClient); //
PubSubClient client(wifiClient);

//define your default values here, if there are different values in config.json, they are overwritten.
char tb_server[20];
char tb_token[20];

#define ThingsBoardPort 20450          // port for thingsboard server, normally 8080 
#define MQTTport 1883                 // as per your setup, normally 1883

#define TB_POST_CELL_INTERVAL 1000
#define TB_POST_DATA_INTERVAL 1500

bool sendEmailWithParam = false;
/*
  #define GPIO_X  // choose ESP pin
  #define GPIO_Y  // choose ESP pin

  #define GPIO_X_PIN  // choose TB pin
  #define GPIO_Y_PIN  // choose TB pin
*/

// We assume that all GPIOs are LOW
boolean gpioState[] = {false, false};


/*
  // Processes function for RPC call "example_set_temperature"
  // RPC_Data is a JSON variant, that can be queried using operator[]
  // See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
  RPC_Response processFirmwareUpdate(const RPC_Data &data)
  {
  Serial.println("Received the set temperature RPC method");

  // Process data

  float example_firmware = data["firmware"];

  Serial.print("Example Firmware: ");
  Serial.println(example_firmware);

  // Just an response example
  return RPC_Response("example_response", 42);
  }

  // Processes function for RPC call "example_set_switch"
  // RPC_Data is a JSON variant, that can be queried using operator[]
  // See https://arduinojson.org/v5/api/jsonvariant/subscript/ for more details
  RPC_Response processDeepSleep(const RPC_Data &data)
  {
  Serial.println("Received the set switch method");

  // Process data

  bool switch_state = data["switch"];

  Serial.print("Example switch state: ");
  Serial.println(switch_state);

  // Just an response example
  return RPC_Response("example_response", 22.02);
  }

  const size_t callbacks_size = 2;

  RPC_Callback callbacks[callbacks_size] = {
  { "example_firmware_update",    processFirmwareUpdate },
  { "example_set_switch",         processDeepSleep }
  };

  bool subscribed = false;
*/
/*************************************************************************************
                                   Config Portal Button
*************************************************************************************/

#include "Ticker.h"
#include "OneButton.h"

//flag for saving data
bool shouldSaveConfig = false;
bool ConfigPortal = false;
bool ESP_doUpdate = false;
bool TB_doUpdate = false;
bool UpdateNow = false;
bool Update_Available = false;
bool CheckForUpdates = true;

const int CONFIG_PIN = 5; // GPIO pin of ESP-12 Trigger for putting up a configuration portal.
//const int OTA_PIN = 5; // GPIO pin of ESP-12
//const int LED_PIN = 13;   // GPIO pin of ESP-12

// Setup a new OneButton on pin CONFIG_PIN. Hardware dependant.
OneButton button_portal_ota(CONFIG_PIN, true, true);
//OneButton buttonOTA(OTA_PIN, true, true);
Ticker buttonTicker;

void ConfigPortalRequested() { //Called on every tick during a long button press
  Serial.println(button_portal_ota.getPressedTicks());
  ConfigPortal = true;
}

void OtaUpdateRequested() { //Called on a double press
  //Serial.println(button_portal_ota.getPressedTicks());
  ESP_doUpdate = true;
}

void checkButton() { //Called on ticker event. Update button state.
  button_portal_ota.tick();
  //buttonOTA.tick();
}

/*************************************************************************************
                                   MCP2515 Init
*************************************************************************************/

#include <mcp_can.h>
#include <SPI.h>

#define CAN1_INT_PIN 4   // D5 // MCP2515 (1) INT 

#define CAN1_CS_PIN  2   // D4 // MCP2515 (1) CS

#define CLK_PIN  14  // D5 SPI PIN IS STATIC HARDWARE Just for refference

#define MISO_PIN 12  // D6 SPI PIN IS STATIC HARDWARE Just for refference DOUT = INPUT From MCP device

#define MOSI_PIN  13  // D7 SPI PIN IS STATIC HARDWARE Just for refference DIN = Output To MCP device

long unsigned int message_id;                     // AKA rxId;
unsigned char len = 0;
unsigned char message_data[8];                   // AKA rxBuf[8];

bool CAN_ACTIVE = false;
#define CAN_ACTIVE_INTERVAL 1200


MCP_CAN CAN1(2);                               // Set CS to pin 15

/*************************************************************************************
                                  OTA Update Init
*************************************************************************************/

#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

//byte mac[6];
String String_MAC;
uint8_t mac[6];
char macAddr[14];
char macAddrTB[50] = {0};

#define ClientID "build_6f72696f6e_"
#define ClientFirmwareVersion "201113_1.0.2"
#define FirmwareVersion ClientID ClientFirmwareVersion
String Firmware = FirmwareVersion;

//char* update_server "82.76.211.109"
char*  Update_Server;
#define Update_URI "/indexmd5.php"
#define OTA_Port 1234                 // as per your setup, normally 80

#define SSIDBASE 200
#define PASSWORDBASE 220

/*************************************************************************************
                                   NEOPIXEL Init
*************************************************************************************/
#include <Adafruit_NeoPixel.h>

// Digital IO pin connected to the button. This will be driven with a
// pull-up resistor so the switch pulls the pin to ground momentarily.
// On a high -> low transition the button press logic will execute.
#define PIXEL_PIN 1  // Digital IO pin connected to the NeoPixels. //Pin 1
#define PIXEL_COUNT 1  // Number of NeoPixels
//#define CALIBRATIONTIME 20000

// Declare our NeoPixel strip object:
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, NEO_RGBW + NEO_KHZ800);
// Argument 1 = Number of pixels in NeoPixel strip
// Argument 2 = Arduino pin number (most are valid)
// Argument 3 = Pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

unsigned long pixelsInterval = 20; // the time we need to wait
unsigned long rainbowCyclesPreviousMillis = 0;
unsigned long theaterChaseRainbowPreviousMillis = 0;
int theaterChaseRainbowCycles = 0;
int rainbowCycleCycles = 0;
int theaterChaseRainbowQ = 0;

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return pixel.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return pixel.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return pixel.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

/*************************************************************************************
                                  Other
*************************************************************************************/

int status = WL_IDLE_STATUS;
long rssi;

#define DeepSleep 16

unsigned long currentTimeLoop;
unsigned long lastDataSend = 0;
unsigned long lastCellSend = 0;
unsigned long lastCanActive = 0;


char inChar;
String inString = "";            // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
boolean serialOn = false;
boolean first_loop = false;

int i;
int j;


int Cell_Number = 0x00;
float* Cell_Voltage; // MyArray
float Pack_Power = 0;
float Pack_Instant_Voltage = 0;
float Pack_Current = 0;
float Pack_SoC = 0;
float Pack_TempH = 0;
float Pack_TempL = 0;
boolean Pack_7S = 0;
boolean Pack_14S = 0;

uint16_t makeWordCAN(uint16_t w);
uint16_t makeWordCAN(byte h, byte l);

/* functions */
uint16_t makeWordCAN(uint16_t w) {
  return w;
}
uint16_t makeWordCAN(uint8_t h, uint8_t l) {
  return (h << 8) | l;
}

/* macro */
#define word(...) makeWordCAN(__VA_ARGS__)



/*************************************************************************************
                                  Save Config Callback
*************************************************************************************/
//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


/*************************************************************************************
                                  Setup Spiffs
*************************************************************************************/
void setupSpiffs() {
  //read configuration from FS json
  Serial.println("mounting FS...");

  //tft.fillScreen(0x0000);
  //tft.fillRect(15, 25, 165, 10, 0x0000);
  //drawText1("Mounting Spiffs...", CULOARE[0]);

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument jsonBuffer(1024);
        DeserializationError error = deserializeJson(jsonBuffer, buf.get());
        if (error)
          return;
        strcpy(tb_server, jsonBuffer["tb_server"]);
        strcpy(tb_token, jsonBuffer["tb_token"]);
        configFile.close();
      }
    }
  } else {
    Serial.println("failed to mount FS");
    //tft.fillRect(15, 25, 165, 10, 0x0000);
    //drawText1("Fail mount Spiffs", CULOARE[0]);
  }
  //tft.fillRect(5, 25, 165, 10, 0x0000);
}

/*************************************************************************************
                                  Thingsboard JSON Method
*************************************************************************************/


void on_message(const char* topic, byte* payload, unsigned int length) {

  Serial.println("On message");
  char json[length + 1];
  strncpy (json, (char*)payload, length);
  json[length] = '\0';
  Serial.print("Topic: ");
  Serial.println(topic);
  Serial.print("Message: ");
  Serial.println(json);


  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, (char*) json);
  if (error)
  {
    Serial.print("ERROR: ");
    Serial.println(error.c_str());
    return;
  }

  String methodName = String((const char*)doc["method"]);//we are converting a constant string into a String object
  if (methodName.equals("getFirmware")) {
    //set_rpc_status(doc["params"]["Firmware"]);//pin and enabled is the things board command
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
    client.publish(responseTopic.c_str(), get_rpc_status().c_str());
    //client.publish("v1/devices/me/rpc/response", get_rpc_status().c_str());

    TB_doUpdate = true;

  }
  else if (methodName.equals("sendEmailWithParam")) {
    //set_rpc_status(doc["params"]["Firmware"]);//pin and enabled is the things board command
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
    client.publish(responseTopic.c_str(), get_rpc_status().c_str());
    //client.publish("v1/devices/me/rpc/response", get_rpc_status().c_str());
    sendEmailWithParam = true;

    String payloadsendEmailWithParam = "{";
    payloadsendEmailWithParam += "\"sendEmailWithParam\":"; payloadsendEmailWithParam += true; payloadsendEmailWithParam += ",";
    payloadsendEmailWithParam += "\"Cell_1\":"; payloadsendEmailWithParam += String(Cell_Voltage[0], 3); payloadsendEmailWithParam += ",";
    payloadsendEmailWithParam += "\"Cell_2\":"; payloadsendEmailWithParam += String(Cell_Voltage[1], 3); payloadsendEmailWithParam += ",";
    payloadsendEmailWithParam += "\"Cell_3\":"; payloadsendEmailWithParam += String(Cell_Voltage[2], 3);
    payloadsendEmailWithParam += "}";
    char attributessendEmailWithParam[150];
    payloadsendEmailWithParam.toCharArray( attributessendEmailWithParam, 150 );
    client.publish( "v1/devices/me/telemetry", attributessendEmailWithParam);
    Serial.println( attributessendEmailWithParam );

    String payloadsendEmailWithParam2 = "{";
    payloadsendEmailWithParam2 += "\"sendEmailWithParam2\":"; payloadsendEmailWithParam2 += true; payloadsendEmailWithParam2 += ",";
    payloadsendEmailWithParam2 += "\"Cell_4\":"; payloadsendEmailWithParam2 += String(Cell_Voltage[3], 3); payloadsendEmailWithParam2 += ",";
    payloadsendEmailWithParam2 += "\"Cell_5\":"; payloadsendEmailWithParam2 += String(Cell_Voltage[4], 3); payloadsendEmailWithParam2 += ",";
    payloadsendEmailWithParam2 += "\"Cell_6\":"; payloadsendEmailWithParam2 += String(Cell_Voltage[5], 3); payloadsendEmailWithParam2 += ",";
    payloadsendEmailWithParam2 += "\"Cell_7\":"; payloadsendEmailWithParam2 += String(Cell_Voltage[6], 3);
    payloadsendEmailWithParam2 += "}";
    char attributessendEmailWithParam2[150];
    payloadsendEmailWithParam2.toCharArray( attributessendEmailWithParam2, 150 );
    client.publish( "v1/devices/me/telemetry", attributessendEmailWithParam2);
    Serial.println( attributessendEmailWithParam2 );

    //    String payloadEmail = "{";
    //    payloadEmail += "\"sendEmailWithParam\":"; payloadEmail += sendEmailWithParam;
    //    payloadEmail += "}";
    //    char attributesEmail[50];
    //    payloadEmail.toCharArray( attributesEmail, 50 );
    //    client.publish( "v1/devices/me/telemetry", attributesEmail);
    //    Serial.println( attributesEmail );


    sendEmailWithParam = false;
  }
  /*  //ONLY IF YOU WANT GPIO CONTROL
    else if (methodName.equals("setGpioStatus")) {
    set_gpio_status(doc["params"]["pin"], doc["params"]["enabled"]);//pin and enabled is the things board command
    String responseTopic = String(topic);
    responseTopic.replace("request", "response");
    client.publish(responseTopic.c_str(), get_gpio_status().c_str());
    client.publish("v1/devices/me/attributes", get_gpio_status().c_str());
    }
    else if (methodName.equals("getGpioStatus")) {                 //if methodName is equale to getGpioStatus
    String responseTopic = String(topic);                  //we declare topic up as const char* topic// now we are converting a constant string into a String object
    responseTopic.replace("request", "response");          //string.replace(substring1, substring2)
    client.publish(responseTopic.c_str(), get_gpio_status().c_str());
    client.publish("v1/devices/me/attributes", get_gpio_status().c_str());
    }
  */
}


/*************************************************************************************
                                  Thingsboard RPC STATUS
*************************************************************************************/
String get_rpc_status() {

  DynamicJsonDocument doc(1024);
  doc["Firmware_Version"] = FirmwareVersion;
  //Looks like: strPayload: {"Firmware_Version":"build_73746572636f6d_200902_X.X.X"}
  //RPC Method Params:
  //{
  //"firmware_version": "1.0.1"
  //}

  char payload[256];
  serializeJson(doc, payload, sizeof(payload));
  String strPayload = String(payload);

  Serial.print("Payload: ");
  Serial.println(String(payload));

  Serial.print("strPayload: ");
  Serial.println(String(strPayload));


  Serial.print("\n");

  return strPayload;

}

/*************************************************************************************
                                  Thingsboard GPIO STATUS
*************************************************************************************/
/*
  String get_gpio_status() {
  // Prepare gpios JSON payload string
  DynamicJsonDocument doc(1024);
  doc[String(GPIO_X_PIN)] = gpioState[0] ? true : false;

  char payload[256];
  serializeJson(doc, payload, sizeof(payload));
  String strPayload = String(payload);
  Serial.print("Get gpio status: ");
  Serial.println(strPayload);
  return strPayload;
  }


  void set_gpio_status(int pin, boolean enabled) {
  if (pin == GPIO_X_PIN) {
    // Output GPIOs state
    digitalWrite(GPIO_X, enabled ? HIGH : LOW);
    // Update GPIOs state
    gpioState[0] = enabled;
  } else if (pin == GPIO_Y_PIN) {
    // Output GPIOs state
    digitalWrite(GPIO_Y, enabled ? HIGH : LOW);
    // Update GPIOs state
    gpioState[1] = enabled;
  }
  }
*/
/*============================================================================
  ================================ Void Setup ================================
  ============================================================================*/
void setup()
{

  //Serial.begin(115200);
  pinMode(PIXEL_PIN, FUNCTION_3); //UNCOMMENT THIS AND SET PIXEL_PIN TO PIN 1
  pinMode(DeepSleep, INPUT);
  //digitalWrite(PIXEL_PIN, LOW);




  //== == == ==  NEOPIXEL Initialisation == == == ==
  //pinMode(PIXEL_PIN, FUNCTION_3); //UNCOMMENT THIS AND SET PIXEL_PIN TO PIN 1
  pixel.begin(); // Initialize NeoPixel strip object (REQUIRED)
  colorWipe(pixel.Color(  0,   0,   200), 50);    // Black/off
  pixel.show();            // Turn OFF all pixels ASAP
  pixel.setBrightness(70); // Set BRIGHTNESS to about 1/5 (max = 255)
  //pinMode(PIXEL_PIN, OUTPUT);
  //================================================

  delay(100);
  // == == == ==  DISPLAY Initialisation == == == ==

  //tft.initR(INITR_MINI160x80);
  // OR use this initializer (uncomment) if using a 1.14" 240x135 TFT:
  tft.init(135, 240);           // Init ST7789 240x135
  tft.setRotation(1);
  //tft.setFont(&FreeMono9pt7b);
  tft.fillScreen(0x0000);

  delay(100);
  BMP_pCloud_Logo();
  for (i = 0; i < 20; i++) {

    BMP_LoadingBar();
    delay(150);
  }
  tft.fillScreen(0x0000);

  //  BMP_pCloud_Logo();
  //  unsigned long logo_time = 0;
  //  while (logo_time < 3000) {
  //    logo_time = millis();
  //    unsigned long currentTimeSetup = millis();
  //    if (currentTimeSetup - previousTimeLoading >= TFT_LoadingBar_Event ) {
  //      BMP_LoadingBar();
  //      previousTimeLoading = currentTimeSetup;
  //    }
  //  }
  //  delay(150);
  //  tft.fillScreen(0x0000);

  //================================================


  Serial.println("Starting pCloud...");


  //Serial.print("pCloud MAC Address: ");
  //printMacAddress();
  //delay(2000);






  //  if (currentTimeSetup - previousTimeLoading >= TFT_LoadingBar_Event ) {
  //    BMP_LoadingBar();
  //    previousTimeLoading = currentTimeSetup;
  //  }


  //Serial.println(WiFi.macAddress());
  //String_MAC = String(WiFi.macAddress(),HEX);
  //Serial.println(String_MAC);
  //uint32_t yolo = ESP.getChipId();
  //Serial.println(yolo);

  //pinMode(OTA_PIN, INPUT_PULLUP);

  //== == == == == == == == CAN BUS Initialisation == == == == == == == ==

  if (CAN1.begin(MCP_STD, CAN_250KBPS, MCP_16MHZ) == CAN_OK) {   // or MCP_STD or MCP_ANY
    Serial.print("CAN BUS Initialisation Successful!\r\n");
  }
  else {
    Serial.print("CAN BUS Initialisation Failed!\r\n");
  }

  pinMode(CAN1_INT_PIN, INPUT_PULLUP);     // Configuring pin for /INT input MAYBE PUT BACK TO INPUT IF CAN DOES NOT WORK

  // Enable interrupts for the CAN1_INT pin (should be pin 2 or 3 for Uno and other ATmega328P based boards)
  //attachInterrupt(digitalPinToInterrupt(CAN1_INT_PIN), ISR_CAN, FALLING);


  // Avoid overloading the processor or the MCP or the SPI bus by filtering
  // just diagnostics. this could in fact be reduced to only the requested
  // answer frame ID. Remember there are about 1600 fps on the bus

  //CAN1.init_Mask(0, 0, 0x00370000);              // Init first mask...
  //CAN1.init_Mask(1, 0, 0x00370000);              // Init second mask...
  CAN1.init_Mask(0, 0, 0x07FF0000);              // Init first mask...
  CAN1.init_Mask(1, 0, 0x07FF0000);              // Init second mask...

  CAN1.init_Filt(0, 0, 0x00350000);              // Init first filter...
  CAN1.init_Filt(1, 0, 0x00360000);              // Init second filter...

  CAN1.init_Filt(2, 0, 0x00350000);              // Init third filter...
  CAN1.init_Filt(3, 0, 0x00350000);              // Init fouth filter...
  CAN1.init_Filt(4, 0, 0x00350000);              // Init fifth filter...
  CAN1.init_Filt(5, 0, 0x00350000);              // Init sixth filter...


  Serial.println("MCP2515 Library Mask & Filter Example...");
  CAN1.setMode(MCP_NORMAL);                // Change to normal mode to allow messages to be transmitted

  //========================================================================


  //== == == == Config Portal Button Initialisation == == == ==

  // link the ConfigurationPortal functions to button events.
  button_portal_ota.attachDoubleClick(OtaUpdateRequested); //a button press starts a configuration portal while connecting to WiFi
  button_portal_ota.attachDuringLongPress(ConfigPortalRequested); //long press starts a configuration portal while connecting to WiFi or Double Press to check for Software Updates
  //Start timer to detect click events
  buttonTicker.attach(0.05, checkButton);
  //=======================================================



  //== == == ==  Spiffs Initialisation == == == ==

  setupSpiffs();

  //==============================================


  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_tb_server("server", "SERVER", tb_server, 20);
  WiFiManagerParameter custom_tb_token("token", "TOKEN", tb_token, 20);


  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //set static ip
  //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

  //add all your parameters here
  wifiManager.addParameter(&custom_tb_server);
  wifiManager.addParameter(&custom_tb_token);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);


  //== == == == == == == == == == == CONNECT TO WiFi == == == == == == == == == == ==
  // Check if pCloud has any configured SSID to connect to
  // If not, then open a config portal to configure them
  if (WiFi.SSID() == "") {
    Serial.println("We haven't got any Access Point credentials, Opening Config Portal...");
    ConfigPortal = true;
  }
  else {
    WiFi.mode(WIFI_STA); // Force to station mode because if device was switched off while in access point mode it will start up next time in access point mode.
    Serial.println("STATION MODE ON");
  }

  int Counter_WiFiSearch = 0;
  unsigned long WiFi_StartedMillis = millis();

  int status = WL_DISCONNECTED;
  //while ((status != WL_CONNECTED) && (ConfigPortal== false) && (Counter_WiFiSearch < 150)) { //Move on after 5 minutes if connection fails.

  //Wait for WiFi to get connected
  while ((status != WL_CONNECTED) && (ConfigPortal == false)) { //Move on after 5 minutes if connection fails.
    unsigned long WiFi_StartedMillis_2 = millis();
    status = WiFi.status();
    Serial.print(status);
    Serial.print(" : ");
    Serial.println(Counter_WiFiSearch);
    if (WiFi_StartedMillis_2 - previousTimeLoading >= TFT_LoadingBar_Event ) {
      BMP_LoadingBar();
      previousTimeLoading = WiFi_StartedMillis_2;
      // Serial.println(currentTimeSetup);
    }
    switch (status)
    {
      // WL_NO_SHIELD 255
      //WL_SCAN_COMPLETED 2
      //WL_CONNECTION_LOST 5
      case WL_IDLE_STATUS: { //0
          Serial.println("Status: (0) WL_IDLE_STATUS");
          Serial.println("Device WiFi Failed");
          String text = WiFi.SSID();
          text = text + " not visible";
          Serial.println(text);
          Serial.println("Push Button to Change/Configure WiFi");

          drawText1("  WiFi Status: Idle", ST77XX_WHITE);

          drawText2("  Trying to connect...", ST77XX_WHITE);
          break;
        }
      case WL_DISCONNECTED: { // 6
          Serial.println("Status: (6) WL_DISCONNECTED");
          Serial.println("Connecting to WiFi");
          String text = WiFi.SSID();
          Serial.println(text);
          Serial.println("Push Button to Change/Configure WiFi");

          drawText1("WiFi Status: Disconnected", ST77XX_WHITE);

          drawText2(" Trying to connect...", ST77XX_WHITE);
          break;
        }
      case WL_NO_SSID_AVAIL: { // 1
          Serial.println("Status: (1) WL_NO_SSID_AVAIL");
          Serial.println("Connecting to WiFi");
          String text = WiFi.SSID();
          text = text + " not visible";
          Serial.println(text);
          Serial.println("Push Button to Change/Configure WiFi");

          drawText1("WiFi Status: No SSID", ST77XX_WHITE);

          drawText2(" Trying to connect...", ST77XX_WHITE);
          break;
        }
      case WL_CONNECTED: {  // 3
          //if you get here you have connected to the WiFi
          Serial.println("WiFi Connected");

          tft.fillScreen(0x0000);

          drawText3("    WiFi ", ST77XX_GREEN);
          drawText4(" Connected", ST77XX_GREEN);

          //read updated parameters
          strcpy(tb_server, custom_tb_server.getValue());
          strcpy(tb_token, custom_tb_token.getValue());


          //save the custom parameters to FS
          if (shouldSaveConfig) {
            Serial.println(F("Saving Config"));
            DynamicJsonDocument jsonBuffer(1024);
            jsonBuffer["tb_server"] = tb_server;
            jsonBuffer["tb_token"] = tb_token;

            File configFile = SPIFFS.open("/config.json", "w");
            if (!configFile) {
              Serial.print(F("failed to open config file for writing"));
            }
            serializeJson(jsonBuffer, Serial);
            Serial.println();
            serializeJson(jsonBuffer, configFile);
            configFile.close();
            //end save
          }
          break;
        }
      case WL_CONNECT_FAILED: { //4
          Serial.println("Status: (4) WL_CONNECT_FAILED");
          Serial.println("Connecting to WiFi");
          Serial.println("Connect Failed");
          Serial.println("Try Restarting");

          drawText1("Status: Connection Failed", ST77XX_WHITE);
          drawText2(" Trying to connect...", ST77XX_WHITE);
          //drawText2("Try Restarting", ST77XX_RED);
          break;
        }

        Serial.println("Connecting to WiFi");
    }
    Counter_WiFiSearch++;

    if (Counter_WiFiSearch >= 60) ESP.restart();  //Temporary, maybe permanent

    delay(500);
  }
  Serial.print("After waiting ");
  float TimeWaited = (millis() - WiFi_StartedMillis);
  Serial.print(TimeWaited / 1000);
  Serial.print(" secs in setup() connection result is ");
  Serial.println(WiFi.status());


  client.setServer( tb_server, MQTTport );

  client.setCallback(on_message);

  //    if (!client.subscribe("v1/devices/me/rpc/request/+")) {
  //      Serial.println("Subscribing for RPC...");
  //      //client.subscribe("v1/devices/me/rpc/request/+");
  //      Serial.println("Subscribe Done");
  //    }

  Update_Server = tb_server;

  Serial.println("Checking for updates...");
  tft.fillScreen(0x0000);

  BMP_Check_Update();
  //drawText3("Checking for ", ST77XX_GREEN);
  drawText5("Update Check", 0x05DD);


  iotUpdater(true);

  delay(500);

  //UpdateNow = false;

  while (!client.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.print("Connecting to Thingsboard node ...");
    // Attempt to connect (clientId, username, password)
    //if ( client.connect(thingsboardServer, TOKEN, NULL) ) {
    if ( client.connect(tb_server, tb_token, NULL) ) {
      Serial.println( "[DONE]" );
      if (!client.subscribe("v1/devices/me/rpc/request/+")) {
        Serial.println("Subscribing for RPC...");
        //client.subscribe("v1/devices/me/rpc/request/+");
        Serial.println("Subscribe Done");
      }
    }
  }

  String payloadUpdate = "{";
  payloadUpdate += "\"Update_Available\":"; payloadUpdate += Update_Available; payloadUpdate += ",";
  payloadUpdate += "\"Firmware_Version\":"; payloadUpdate += ClientFirmwareVersion;
  payloadUpdate += "}";
  char attributesUpdate[100];
  payloadUpdate.toCharArray( attributesUpdate, 100 );
  client.publish( "v1/devices/me/telemetry", attributesUpdate);
  Serial.println( attributesUpdate );
  //delay (200);

  tft.fillScreen(0x0000); //Clear screen

  if (Update_Available == true) {
    BMP_S_New_Update_Cloud();
    BMP_S_New_Update_Arrow();
    //BMP_NEW_Update();
  }
  else {
    tft.fillRect(120, 0, 28, 23, 0x0000);
  }
  BMP_BatteryBig_Setup();

  //  == == == ==  NEOPIXEL Initialisation == == == ==
  //pinMode(PIXEL_PIN, FUNCTION_3); //UNCOMMENT THIS AND SET PIXEL_PIN TO PIN 1
  pixel.begin(); // Initialize NeoPixel strip object (REQUIRED)
  pixel.show();            // Turn OFF all pixels ASAP

  pixel.setBrightness(70); // Set BRIGHTNESS to about 1/5 (max = 255)
  //pinMode(PIXEL_PIN, OUTPUT);
  //================================================

  //BMP_BatteryBig();
  // Set the ConfigurationPortal function to be called on a LongPress event.
  //This callback will fire every tick so to avoid simultaneous instances don't do much there
}




/*============================================================================
  ================================ Void Loop =================================
  ============================================================================*/

void loop()
{
  currentTimeLoop = millis();
  status = WiFi.status();
  //WiFiManager wifiManager;

  //  if (CheckForUpdates == true) {
  //    CheckForUpdates = false;
  //    iotUpdater(true);
  //  }

  if ((unsigned long)(millis() - rainbowCyclesPreviousMillis) >= pixelsInterval) {
    rainbowCyclesPreviousMillis = millis();
    rainbowCycle();
  }
  // == == == == == == == UPDATE WIFI SIGNAL STRENGTH == == == == == == ==


  if (currentTimeLoop - previousTimeWiFi >= TFT_WiFi_Event ) {
    rssi = WiFi.RSSI();
    TFT_WiFi_Strength =  getRSSIasQuality(rssi);
    BMP_WiFi();
    previousTimeWiFi = currentTimeLoop;
  }

  //  Serial.print("RSSI:");
  //  Serial.println( getRSSIasQuality(rssi));



  //Serial.print( client.state() );
  /*
    if ( (digitalRead(CONFIG_PIN) == LOW) || (ConfigPortal == true)) {
      ConfigPortal = true;
      ConfigPortalRequired();
    }
  */
  //  if (digitalRead(OTA_PIN) == LOW && WiFi.status() == WL_CONNECTED ){
  //    iotUpdater(true);
  //  }

  // == == == == == == == CHECK FOR UPDATE == == == == == == ==
  if ((WiFi.status() == WL_CONNECTED && ESP_doUpdate == true) ||  (WiFi.status() == WL_CONNECTED && TB_doUpdate == true)) {
    OtaUpdateRequired();
  }

  if (ConfigPortal) {
    ConfigPortalRequired();
  }

  // == == == == == == == CHECK WIFI CONNECTION == == == == == == ==
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connection Lost");
    WiFiReconnect();

    Serial.println("Connected to Access Point: ");
    Serial.println(WiFi.SSID());
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
  }

  // == == == == == == == CHECK SERVER CONNECTION == == == == == == ==
  else if ( client.state() != 0 && WiFi.status() == WL_CONNECTED) {
    ClientReconnect();
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);
  }

  // == == == == == == == READ CAN BUS DATA == == == == == == ==
  else if ( client.connected() && WiFi.status() == WL_CONNECTED) {
    // Loading Bar While CAN Bus is working
    //    tft.fillRect(5, 25, 165, 10, ST7735_RED);
    //drawText1("CAN BUS ", ST7735_WHITE);
    //    if (currentTimeLoop - previousTimeLoading >= TFT_LoadingBar_Event ) {
    //      BMP_LoadingBar();
    //      previousTimeLoading = currentTimeLoop;
    //      // Serial.println(currentTime);
    //    }


    if (CAN_MSGAVAIL == CAN1.checkReceive())           // check if data coming
    {
      //If can was OFF, then it went back ON, we need to first clear the screen before saying can ON.
      if (!CAN_ACTIVE) {
        tft.fillRect(80, 15, 62, 22, 0x0000);
      }
      BMP_CAN();    //Display CAN LOGO
      BMP_CAN_ON(); //Display CAN LOGO
      CAN_ACTIVE = true;
      lastCanActive = millis();

      CAN_BUS_DATA();

    }
    if (CAN_ACTIVE == true && millis() - lastCanActive > CAN_ACTIVE_INTERVAL ) {

      CAN_ACTIVE = false;
      tft.fillRect(80, 15, 62, 22, 0x0000);
      BMP_CAN();     //Display CAN LOGO
      BMP_CAN_OFF(); //Display CAN LOGO
    }
    //== == == == == == SHOW SoC ON DISPLAY == == == == == ==
    if (currentTimeLoop - previousTimeBattery >= TFT_Battery_Event) {
      //ROUNDED SOC
      /*
        ROUND_TFT_SOC = Pack_SoC;
        ROUND_TFT_SOC = ROUND_TFT_SOC / 5;
        ROUND_TFT_SOC = (long)(ROUND_TFT_SOC * 10) / 10; // We take only the integer part and round it up
        ROUND_TFT_SOC = ROUND_TFT_SOC * 5;
        if (OLD_TFT_SOC != ROUND_TFT_SOC) {

        OLD_TFT_SOC = ROUND_TFT_SOC;
      */

      //REAL SOC
      ROUND_TFT_SOC = Pack_SoC;
      ROUND_TFT_SOC = (long)(ROUND_TFT_SOC * 10) / 10; // We take only the integer part and round it up

      if (OLD_TFT_SOC != Pack_SoC) {

        OLD_TFT_SOC = Pack_SoC;
        //OLD_TFT_SOC = ROUND_TFT_SOC;
        if (CAN_ACTIVE) {
          BMP_BatteryBig();
        }
        else if (!CAN_ACTIVE) {
          BMP_BatteryBig_Setup();
        }
      }
      previousTimeBattery = currentTimeLoop;
    }
    //===========================================================
    // == == == == == == == UPLOAD CELL DATA ON SERVER == == == == == == ==
    if ( millis() - lastCellSend > TB_POST_CELL_INTERVAL ) { // Update and send only after 1 seconds
      if (CAN_ACTIVE == true) {          // check if data is coming

        UploadCellData();

        lastCellSend = millis();
      }
    }


    // == == == == == == == UPLOAD DATA ON SERVER == == == == == == ==
    if ( millis() - lastDataSend > TB_POST_DATA_INTERVAL ) { // Update and send only after 1 seconds

      UploadData();

      lastDataSend = millis();
    }
  }

  client.loop();

  yield(); // Maybe remove this if strange behaviour occurs
}


/*************************************************************************************
                                     CAN BUS DATA
*************************************************************************************/
// if (CAN_MSGAVAIL == CAN1.checkReceive())           // check if data coming ADD: else{ make all values 0 and post them

void CAN_BUS_DATA() {

  if (!digitalRead(CAN1_INT_PIN))                   // If pin 2 is low, read receive buffer
  {
    CAN1.readMsgBuf(&message_id, &len, message_data); // Read data: len = data length, buf = data byte(s)
    Serial.print("ID: ");
    Serial.print(message_id, HEX);
    Serial.print(" Data: ");
    for (int i = 0; i < len; i++)        // Print each byte of the data
    {
      if (message_data[i] < 0x10)               // If data byte is less than 0x10, add a leading zero
      {
        Serial.print("0");
      }
      Serial.print(message_data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  }
  if (message_id == 0x36) //uncomment when you want to filter
  {
    Cell_Number = word(message_data[0]);  // Because first Cell is read as 0, we add 1 to the variable
    Serial.println(" ");
    Serial.print("Cell Number: ");
    Serial.println(Cell_Number + 1);

    if  (Cell_Number == 6 && Pack_7S == 0) {
      delete [] Cell_Voltage;
      Cell_Voltage = new float [6];
      Serial.println("7S Detected, Allocating New Array Size");
      Pack_7S = 1;
    }
    else if (Cell_Number == 13 && Pack_14S == 0) {
      delete [] Cell_Voltage;
      Cell_Voltage = new float [13];
      Serial.println("14S Detected, Allocating New Array Size");
      Pack_14S = 1;
    }

    if (Pack_14S == 1) {
      //Serial.println("14S Pack");
      Cell_Voltage[Cell_Number] = word(message_data[1], message_data[2]) / 10000.0;
      Serial.print("Cell Voltage: ");
      Serial.println(Cell_Voltage[Cell_Number], 3);
    }

    else if (Pack_7S == 1 && Pack_14S == 0) {
      //Serial.println("7S Pack");
      Cell_Voltage[Cell_Number] = word(message_data[1], message_data[2]) / 10000.0;
      Serial.print("Cell Voltage: ");
      Serial.println(Cell_Voltage[Cell_Number], 3);

      //        Serial.print("Cell Voltages: ");
      //        Serial.print(Cell_Voltage[0], 3);
      //        Serial.print(" ");
      //        Serial.print(Cell_Voltage[1], 3);
      //        Serial.print(" ");
      //        Serial.print(Cell_Voltage[2], 3);
      //        Serial.print(" ");
      //        Serial.print(Cell_Voltage[3], 3);
      //        Serial.print(" ");
      //        Serial.print(Cell_Voltage[4], 3);
      //        Serial.print(" ");
      //        Serial.print(Cell_Voltage[5], 3);
      //        Serial.print(" ");
      //        Serial.print(Cell_Voltage[6], 3);
      //        Serial.println(" ");
    }
  }

  //pack_voltage = ((highByte(message_data[1]) << 8 | lowByte(message_data[2])) / 10.0);
  if (message_id == 0x35 ) //uncomment when you want to filter
  {
    Pack_Instant_Voltage = word(message_data[1], message_data[2]) / 10.0;
    Pack_Current = word(message_data[3], message_data[4]) / 10.0;
    Pack_Power = Pack_Instant_Voltage * Pack_Current;
    Pack_SoC = word(message_data[5]) / 10.0;
    Pack_TempH = word(message_data[6]);
    Pack_TempL = word(message_data[7]);


    //      Serial.print("Pack Instant Voltage: ");
    //      Serial.println(Pack_Instant_Voltage, 2);
    //      Serial.print("Pack Current: ");
    //      Serial.println(Pack_Current);
    //      Serial.print("Pack SoC: ");
    //      Serial.println(Pack_SoC);
    //      Serial.print("Pack Temp High: ");
    //      Serial.println(Pack_TempH);
    //      Serial.print("Pack Temp Low: ");
    //      Serial.println(Pack_TempL);
  }

}

/*************************************************************************************
                                     WiFi Reconnect
*************************************************************************************/

void WiFiReconnect()
{
  //int Counter_WiFiSearch = 0;

  int Rec = 0;
  BMP_NoWiFi();

  WiFi.begin( WiFi.SSID(), WiFi.psk());
  Serial.println("Attempting to reconnect to WiFi Network...");

  colorWipe(pixel.Color(  0,   200,   0), 50);    // Green, Red, Blue
  pixel.show();            // Turn OFF all pixels ASAP
  //while (Counter_WiFiSearch < 100) {
  while (WiFi.status() != WL_CONNECTED) {
    /*if ( digitalRead(CONFIG_PIN) == LOW ) {
      ConfigPortal = true;
      ConfigPortalRequired();
      //Counter_WiFiSearch = 100;
      }*/
    if (ConfigPortal == true) {
      ConfigPortalRequired();
    }
    delay(500);
    Serial.print(".");
    Rec++;
    if (Rec >= 60)
    {
      ESP.restart();  //Temporary, maybe permanent
    }
    //Counter_WiFiSearch++;
  }
}

/*************************************************************************************
                                  Client Reconnect
*************************************************************************************/
void ClientReconnect() {
  // Loop until we're reconnected

  while (!client.connected() && WiFi.status() == WL_CONNECTED) {
    Serial.print("Connecting to Thingsboard node ...");
    // Attempt to connect (clientId, username, password)
    //if ( client.connect(thingsboardServer, TOKEN, NULL) ) {
    if ( client.connect(tb_server, tb_token, NULL) ) {
      Serial.println( "[DONE]" );
      if (!client.subscribe("v1/devices/me/rpc/request/+")) {
        //client.subscribe("v1/devices/me/rpc/request/+");
      }
    }
    if (ConfigPortal) {
      break;
    }
    Serial.print( "[FAILED] [ rc = " );
    Serial.print( client.state() );
    Serial.println( " : retrying in 1 seconds]" );
    // Wait 3 seconds before retrying
    delay( 1000 );
  }
}


/*************************************************************************************
                                  Read Serial Data
*************************************************************************************/
void ReadSerialData() {
  while (Serial.available()) {
    serialOn = true;
    char inChar = (char)Serial.read();
    inString += inChar;
    if (inChar == ';') {
      stringComplete = true;
    }
  }
}

/*************************************************************************************
                                  Upload Data
*************************************************************************************/
void UploadData() {
  /*
    if (CAN_ACTIVE && client.connected()) {
    String payloadActive = " {";
    payloadActive += "\"Active\":"; payloadActive += 1;
    payloadActive += "}";
    char attributesActive[15];
    payloadActive.toCharArray( attributesActive, 15 );
    client.publish( "v1/devices/me/telemetry", attributesActive);
    Serial.println( attributesActive );
    }*/



  if (!CAN_ACTIVE) {
    Pack_Instant_Voltage = 0;
    Pack_Current = 0;
    Pack_Power = 0;
    Pack_SoC = 0;
    Pack_TempH = 0;
    Pack_TempL = 0;
  }

  String payload_Data_1 = "{";
  payload_Data_1 += "\"Pack_Instant_Voltage\":"; payload_Data_1 += Pack_Instant_Voltage;    payload_Data_1 += ",";
  payload_Data_1 += "\"Pack_SoC\":";             payload_Data_1 += Pack_SoC ;               payload_Data_1 += ",";
  payload_Data_1 += "\"Pack_Current\":";         payload_Data_1 += Pack_Current;
  payload_Data_1 += "}";

  String payload_Data_2 = "{";
  payload_Data_2 += "\"Pack_Power\":";           payload_Data_2 += Pack_Power;                payload_Data_2 += ",";
  payload_Data_2 += "\"Pack_TempH\":";           payload_Data_2 += Pack_TempH;                payload_Data_2 += ",";
  payload_Data_2 += "\"Pack_TempL\":";           payload_Data_2 += Pack_TempL;
  payload_Data_2 += "}";

  String payload_Data_3 = "{";
  payload_Data_3 += "\"MAC_Address\":";           payload_Data_3 += String_MAC;                payload_Data_3 += ",";
  payload_Data_3 += "\"Firmware_Version\":";      payload_Data_3 += ClientFirmwareVersion;
  //  payload_Data_3 += "\"Update_Available\":";      payload_Data_3 += Update_Available;
  payload_Data_3 += "}";

  //char Data[6];
  //char str_voltage[6];
  //dtostrf(V_Cell[i], 1, 3, str_voltage);
  //sprintf(Data,"%s", str_voltage);

  char attributesData_1[150];
  payload_Data_1.toCharArray( attributesData_1, 150 );
  client.publish( "v1/devices/me/telemetry", attributesData_1);
  Serial.println( attributesData_1 );

  char attributesData_2[150];
  //sprintf(attributesData_2,"{\"Pack_TempH\":%s,\"Pack_TempL\":%s}", char_pack_temph, char_pack_templ);
  payload_Data_2.toCharArray( attributesData_2, 150 );
  client.publish( "v1/devices/me/telemetry", attributesData_2);
  Serial.println( attributesData_2 );

  char attributesData_3[150];
  payload_Data_3.toCharArray( attributesData_3, 150 );
  client.publish( "v1/devices/me/telemetry", attributesData_3);
  Serial.println( attributesData_3 );


  //    String payloadCAN_On = "{";
  //    payloadCAN_On += "\"CAN_On\":"; payloadCAN_On += 0;
  //    payloadCAN_On += "}";
  //
  //    char attributesCAN_On[15];
  //    payloadCAN_On.toCharArray( attributesCAN_On, 15 );
  //    client.publish( "v1/devices/me/telemetry", attributesCAN_On);
}
/*************************************************************************************
                                  Upload Cell Data
*************************************************************************************/
void UploadCellData() {

  if (Pack_7S == 1) {
    String payload_Cells_1 = "{";
    payload_Cells_1 += "\"Cell_1\":"; payload_Cells_1 += String(Cell_Voltage[0], 3); payload_Cells_1 += ",";
    payload_Cells_1 += "\"Cell_2\":"; payload_Cells_1 += String(Cell_Voltage[1], 3); payload_Cells_1 += ",";
    payload_Cells_1 += "\"Cell_3\":"; payload_Cells_1 += String(Cell_Voltage[2], 3); payload_Cells_1 += ",";
    payload_Cells_1 += "\"Cell_4\":"; payload_Cells_1 += String(Cell_Voltage[3], 3);
    payload_Cells_1 += "}";

    String payload_Cells_2 = "{";
    payload_Cells_2 += "\"Cell_5\":"; payload_Cells_2 += String(Cell_Voltage[4], 3); payload_Cells_2 += ",";
    payload_Cells_2 += "\"Cell_6\":"; payload_Cells_2 += String(Cell_Voltage[5], 3); payload_Cells_2 += ",";
    payload_Cells_2 += "\"Cell_7\":"; payload_Cells_2 += String(Cell_Voltage[6], 3);
    payload_Cells_2 += "}";

    char Cell_Data_1[150];
    payload_Cells_1.toCharArray( Cell_Data_1, 150 );
    //sprintf(attributes,"{\"Cell_1\":%s,\"Cell_2\":%s,\"Cell_3\":%s,\"Cell_4\":%s,\"Cell_5\":%s,\"Cell_6\":%s}", myStrings[0], myStrings[1], myStrings[2], myStrings[3], myStrings[4], myStrings[5]);      //Version 2 of uploading telemetry
    client.publish( "v1/devices/me/telemetry", Cell_Data_1);
    Serial.println( Cell_Data_1 );

    char Cell_Data_2[150];
    payload_Cells_2.toCharArray( Cell_Data_2, 150 );
    //sprintf(attributes,"{\"Cell_7\":%s}", myStrings[6]]);     //Version 2 of uploading telemetry
    client.publish( "v1/devices/me/telemetry", Cell_Data_2);
    Serial.println( Cell_Data_2 );
  }

  else if (Pack_7S == 1 && Pack_14S == 0) {

    String payload_Cells_1 = "{";
    payload_Cells_1 += "\"Cell_1\":"; payload_Cells_1 += String(Cell_Voltage[0], 3); payload_Cells_1 += ",";
    payload_Cells_1 += "\"Cell_2\":"; payload_Cells_1 += String(Cell_Voltage[1], 3); payload_Cells_1 += ",";
    payload_Cells_1 += "\"Cell_3\":"; payload_Cells_1 += String(Cell_Voltage[2], 3); payload_Cells_1 += ",";
    payload_Cells_1 += "\"Cell_4\":"; payload_Cells_1 += String(Cell_Voltage[3], 3); payload_Cells_1 += ",";
    payload_Cells_1 += "\"Cell_5\":"; payload_Cells_1 += String(Cell_Voltage[4], 3); payload_Cells_1 += ",";
    payload_Cells_1 += "\"Cell_6\":"; payload_Cells_1 += String(Cell_Voltage[5], 3);
    payload_Cells_1 += "}";

    String payload_Cells_2 = "{";
    payload_Cells_2 += "\"Cell_7\":"; payload_Cells_2 += String(Cell_Voltage[6], 3); payload_Cells_2 += ",";
    payload_Cells_2 += "\"Cell_8\":"; payload_Cells_2 += String(Cell_Voltage[7], 3); payload_Cells_2 += ",";
    payload_Cells_2 += "\"Cell_9\":"; payload_Cells_2 += String(Cell_Voltage[8], 3); payload_Cells_2 += ",";
    payload_Cells_2 += "\"Cell_10\":"; payload_Cells_2 += String(Cell_Voltage[9], 3); payload_Cells_2 += ",";
    payload_Cells_2 += "\"Cell_11\":"; payload_Cells_2 += String(Cell_Voltage[10], 3); payload_Cells_2 += ",";
    payload_Cells_2 += "\"Cell_12\":"; payload_Cells_2 += String(Cell_Voltage[11], 3);
    payload_Cells_2 += "}";

    String payload_Cells_3 = "{";
    payload_Cells_3 += "\"Cell_13\":"; payload_Cells_3 += String(Cell_Voltage[12], 3); payload_Cells_3 += ",";
    payload_Cells_3 += "\"Cell_14\":"; payload_Cells_3 += String(Cell_Voltage[13], 3); payload_Cells_3 += ",";
    payload_Cells_3 += "\"Cell_15\":"; payload_Cells_3 += String(Cell_Voltage[14], 3); payload_Cells_3 += ",";
    payload_Cells_3 += "\"Cell_16\":"; payload_Cells_3 += String(Cell_Voltage[15], 3); payload_Cells_3 += ",";
    payload_Cells_3 += "\"Cell_17\":"; payload_Cells_3 += String(Cell_Voltage[36], 3); payload_Cells_3 += ",";
    payload_Cells_3 += "\"Cell_18\":"; payload_Cells_3 += String(Cell_Voltage[37], 3);
    payload_Cells_3 += "}";

    char Cell_Data_1[150];
    payload_Cells_1.toCharArray( Cell_Data_1, 150 );
    //sprintf(attributes,"{\"Cell_1\":%s,\"Cell_2\":%s,\"Cell_3\":%s,\"Cell_4\":%s,\"Cell_5\":%s,\"Cell_6\":%s, myStrings[0], myStrings[1], myStrings[2], myStrings[3], myStrings[4], myStrings[5]);      //Version 2 of uploading telemetry
    client.publish( "v1/devices/me/telemetry", Cell_Data_1);
    Serial.println( Cell_Data_1 );

    char Cell_Data_2[150];
    payload_Cells_2.toCharArray( Cell_Data_2, 150 );
    //sprintf(attributes,"{\"Cell_7\":%s,\"Cell_8\":%s,\"Cell_9\":%s,\"Cell_10\":%s,\"Cell_11\":%s,\"Cell_12\":%s, myStrings[6], myStrings[7], myStrings[8], myStrings[9], myStrings[10], myStrings[11]);     //Version 2 of uploading telemetry
    client.publish( "v1/devices/me/telemetry", Cell_Data_2);
    Serial.println( Cell_Data_2 );

    char Cell_Data_3[150];
    payload_Cells_3.toCharArray( Cell_Data_3, 150 );
    //sprintf(attributes,"{\"Cell_13\":%s,\"Cell_14\":%s, myStrings[12], myStrings[13]]);      //Version 2 of uploading telemetry
    client.publish( "v1/devices/me/telemetry", Cell_Data_3);
    Serial.println( Cell_Data_3 );
  }
}

/*************************************************************************************
                                  OtaUpdateRequired
*************************************************************************************/
void OtaUpdateRequired() {

  if (ESP_doUpdate || TB_doUpdate) {
    UpdateNow = true;
    iotUpdater(true);
  }
}

/*************************************************************************************
                                  ConfigPortalRequired
*************************************************************************************/

void ConfigPortalRequired() {
  colorWipe(pixel.Color(  200,   0,   0), 50);    // Green, Red, Blue
  pixel.show();            // Turn OFF all pixels ASAP
  if (ConfigPortal) {
    Serial.println("Configuration portal requested.");
    Serial.println("To configure WiFi. Go to");
    //Serial.println("wifi.urremote.com");
    Serial.println("192.168.4.1");
    Serial.println("Connect to");
    String text = "ESP" + String(ESP.getChipId());
    text = text + " Wifi Network";
    Serial.println(text);
    tft.fillScreen(0x0000);

    drawText1("   Connect to pCloud AP", ST77XX_WHITE);

    drawText2("    Access 192.168.4.1", ST77XX_WHITE);

    // The extra parameters to be configured (can be either global or just in the setup)
    // After connecting, parameter.getValue() will get you the configured value
    // id/name placeholder/prompt default length
    WiFiManagerParameter custom_tb_server("server", "SERVER", tb_server, 20);
    WiFiManagerParameter custom_tb_token("token", "TOKEN", tb_token, 20);
    //ESP.wdtDisable();

    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //WiFi.disconnect();

    //*******************************Temporary*********************************
    //set config save notify callback
    wifiManager.setSaveConfigCallback(saveConfigCallback);

    //set static ip
    //wifiManager.setSTAStaticIPConfig(IPAddress(10,0,1,99), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //add all your parameters here
    wifiManager.addParameter(&custom_tb_server);
    wifiManager.addParameter(&custom_tb_token);
    //*******************************************************************************

    //sets timeout in seconds until configuration portal gets turned off.
    //If not specified device will remain in configuration mode until
    //switched off via webserver or device is restarted.
    //wifiManager.setConfigPortalTimeout(600);

    // we don't want it to connect after being configured ... just reset.
    wifiManager.setBreakAfterConfig(true);

    //wifiManager.setSaveConfigCallback(* ConfigSavedScreen);

    //it starts an access point
    //and goes into a blocking loop awaiting configuration
    //wifiManager.setSaveConfigCallback();
    //CONNECT TO pCloud Access Point
    //Access 192.168.4.1 to configure
    //------------------------------------------------------------------------------------
    WiFi.disconnect(true);
    delay(1000);

    wifiManager.startConfigPortal("pCloud", "PrimeCloud");          //wifiManager.startConfigPortal("ESP_Monitor") if you want to change ESP Name or wifiManager.startConfigPortal("ESP_Monitor", "Parola") if you want to add password too
    Serial.println("pCloud Connected to WiFi!");

    //read updated parameters
    strcpy(tb_server, custom_tb_server.getValue());
    strcpy(tb_token, custom_tb_token.getValue());

    //save the custom parameters to FS
    if (shouldSaveConfig) {
      Serial.println(F("Saving Config"));
      DynamicJsonDocument jsonBuffer(1024);
      jsonBuffer["tb_server"] = tb_server;
      jsonBuffer["tb_token"] = tb_token;
      Serial.print(F("Saving Config"));
      File configFile = SPIFFS.open("/config.json", "w");
      if (!configFile) {
        Serial.print(F("failed to open config file for writing"));
      }
      serializeJson(jsonBuffer, Serial);
      Serial.println();
      serializeJson(jsonBuffer, configFile);
      configFile.close();
      //end save
    }

    /*if (!wifiManager.startConfigPortal()) {
      Serial.println("Not connected to WiFi but continuing anyway.");
      } else {
      //if you get here you have connected to the WiFi
      Serial.println("connected...yeey :)");

      //save the custom parameters to FS
      if (shouldSaveConfig) {
        Serial.print(F("Saving Config"));
        DynamicJsonDocument jsonBuffer(1024);
        jsonBuffer["tb_server"] = tb_server;
        jsonBuffer["tb_token"] = tb_token;

        File configFile = SPIFFS.open("/config.json", "w");
        if (!configFile) {
          Serial.print(F("failed to open config file for writing"));
        }
        serializeJson(jsonBuffer, Serial);
        Serial.println();
        serializeJson(jsonBuffer, configFile);
        configFile.close();
        //end save
      }

      }*/
    ConfigPortal = false; //Turn Flag Off
    delay(3000);
    ESP.reset(); // This is a bit crude. For some unknown reason webserver can only be started once per boot up
    // so resetting the device allows to go back into config mode again when it reboots.
    delay(5000);
  }
}

/*************************************************************************************
                                  Print Mac Address
*************************************************************************************/
void printMacAddress() {
  Serial.print("pCloud MAC Address: ");
  WiFi.macAddress(mac);
  ///sprintf(macAddr,"%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); /// small letters at MAC address
  sprintf(macAddr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); /// capital letters at MAC address
  //sprintf(macAddrTB, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]); /// capital letters at MAC address
  String_MAC = macAddr;
  String_MAC.replace(":", "-");
  //Serial.println(String_MAC);
  Serial.println("\n");
}
/*
  void printMacAddress() {

  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  for (int i = 0; i < 5; i++) {
    Serial.print(mac[i], HEX);
    Serial.print(":");
  }
  Serial.println(mac[5], HEX);
  String_MAC = String(mac[0], HEX) + ":" + String(mac[1], HEX) + ":" + String(mac[2], HEX) + ":" + String(mac[3], HEX) + ":" + String(mac[4], HEX) + ":" + String(mac[5], HEX);
  //Serial.println(String_MAC);
  }
*/
/*************************************************************************************
                                  IoT Updater
*************************************************************************************/
void iotUpdater(bool debug) {
  if (debug) {
    printMacAddress();
    //Serial.println(WiFi.macAddress());
    Serial.println("Start Flashing......");
    Serial.println(Update_Server);
    Serial.println(OTA_Port);
    Serial.println(Update_URI);
    Serial.println(FirmwareVersion);
  }


  if (ESP_doUpdate == true || TB_doUpdate == true) {

    ESP_doUpdate = false;
    TB_doUpdate = false;
    //UpdateNow = true;
    //ESPhttpUpdate.doUpdate(UpdateNow);
  }
  //UpdateNow = true;

  if (UpdateNow && Update_Available) {
    tft.fillScreen(0x0000);
    BMP_Updating_Cloud();
    BMP_Updating_Arrow();
    drawText5(" Updating...", ST77XX_GREEN);
  }

  ESPhttpUpdate.doUpdate(UpdateNow);

  t_httpUpdate_return ret = ESPhttpUpdate.update(Update_Server, OTA_Port, Update_URI, ClientID); //Replaced FirmwareVersion with ClientID to make use only of the md5

  //== == == ==  NEOPIXEL Initialisation == == == ==
  //pinMode(PIXEL_PIN, FUNCTION_3); //UNCOMMENT THIS AND SET PIXEL_PIN TO PIN 1
  pixel.begin(); // Initialize NeoPixel strip object (REQUIRED)
  colorWipe(pixel.Color(  0,   0,   200), 50);    // Black/off
  pixel.show();            // Turn OFF all pixels ASAP

  pixel.setBrightness(70); // Set BRIGHTNESS to about 1/5 (max = 255)
  //pinMode(PIXEL_PIN, OUTPUT);
  //================================================
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      if (debug) Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str());
      tft.fillScreen(0x0000);

      BMP_No_Update();
      drawText5("Update Failed!", ST77XX_RED);
      delay(500);
      tft.fillScreen(0x0000);
      break;


    case HTTP_UPDATE_NO_UPDATES:
      if (debug) Serial.println("HTTP_UPDATE_NO_UPDATES");

      tft.fillScreen(0x0000);
      BMP_No_Update();
      drawText5(" No Update!", ST77XX_RED);
      delay(500);
      break;

    case HTTP_UPDATE_OK:

      break;      if (debug) Serial.println("HTTP_UPDATE_OK");


    case HTTP_UPDATE_AVAILABLE:

      if (debug) Serial.println("HTTP_UPDATE_AVAILABLE");
      Update_Available = true;

      tft.fillScreen(0x0000);
      BMP_New_Update_Cloud();
      BMP_New_Update_Arrow();
      drawText5(" New Update!", ST77XX_GREEN);
      delay(500);
      break;

  }
}

void rainbowCycle() {
  uint16_t i;

  //for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
  for (i = 0; i < pixel.numPixels(); i++) {
    pixel.setPixelColor(i, Wheel(((i * 256 / pixel.numPixels()) + rainbowCycleCycles) & 255));
  }
  pixel.show();

  rainbowCycleCycles++;
  if (rainbowCycleCycles >= 256 * 5) rainbowCycleCycles = 0;
}

void theaterChaseRainbow() {
  for (int i = 0; i < pixel.numPixels(); i = i + 3) {
    pixel.setPixelColor(i + theaterChaseRainbowQ, Wheel( (i + theaterChaseRainbowCycles) % 255)); //turn every third pixel on
  }

  pixel.show();
  for (int i = 0; i < pixel.numPixels(); i = i + 3) {
    pixel.setPixelColor(i + theaterChaseRainbowQ, 0);      //turn every third pixel off
  }
  theaterChaseRainbowQ++;
  theaterChaseRainbowCycles++;
  if (theaterChaseRainbowQ >= 3) theaterChaseRainbowQ = 0;
  if (theaterChaseRainbowCycles >= 256) theaterChaseRainbowCycles = 0;
}


void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < pixel.numPixels(); i++) { // For each pixel in strip...
    pixel.setPixelColor(i, color);         //  Set pixel's color (in RAM)
    pixel.show();                          //  Update strip to match
    delay(wait);                           //  Pause for a moment
  }
}

static void ISR_CAN()
{
  // We don't do anything here, this is just for waking up the microcontroller
}
/*******************************
   Individual Cell Update
        char str_voltage[6];
        dtostrf(Cell_Voltage[Cell_Number], 1, 3, str_voltage);
        char test[150];
        sprintf(test, "{\"Cell_1\":%s}", str_voltage);      //Version 2 of uploading telemetry
        Serial.println( str_voltage );
        client.publish( "v1/devices/me/telemetry", test);
        Serial.println( test );
 ******************************/
