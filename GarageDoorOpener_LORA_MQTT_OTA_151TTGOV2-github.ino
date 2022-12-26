//
//     October 2021 This is the working version!!!!
//
// ESP32 connected to garage door UHF remote:
// Garage is opened either via MQTT 
// or by receiving the LORA signal transmitted
// from the LoRa sender in the car.
// The sketch is OTA enabled
//
// The LoRa receiver is a TTGO LoRa32-Oled 
//
// The LoRa transmitter in the car is a Heltec 151 LoRa Node
//

//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


//Basic OTA Libraries
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define LED_BUILTIN 25

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 868E6

//OLED pins
#define OLED_SDA 21
#define OLED_SCL 22 
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

//packet counter
int counter = 0;

const long freq = 868E6;
const int SF = 9;
const long bw = 125E3;


int lastCounter;

// Replace the next variables with your SSID/Password combination
const char* ssid = "SSID";
const char* password = "PASSWD";
// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.12";
const char* mqtt_server = IPADDRESS";

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;


// LED Pin
const int ledPin = 4;



void setup() {
  
  //BasicOTA SETUP BEGIN

  Serial.begin(115200);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  // Port defaults to 3232
  // ArduinoOTA.setPort(3232);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname("Garage LoRa Receiver");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = a743894a0e4a801fc321232f297a57a5
  // ArduinoOTA.setPasswordHash("a743894a0e4a801fc321232f297a57a5");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();

  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  //BasicOTA SETUP END



  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);     //deactivate Garagedoor UHF sender
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(8, OUTPUT);    //A14 opin of heltec 151 node is arduino pin D28, used to open the garage door
  digitalWrite(8, HIGH);     //deactivate Garagedoor UHF sender
 //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    //for(;;); // Don't proceed, loop forever
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("GARAGE DOOR AUTO MQTT ");
  display.display();
  
  Serial.println("LoRa Sender Test");
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);
  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
    LoRa.setSpreadingFactor(SF);
  Serial.println("LoRa Initializing OK!");
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("GARAGE DOOR AUTO         MQTT LORA OK");
  display.display();
  delay(200);
}

void setup_wifi() {
  
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
 
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(),"MQTTUSER","MQTTPASSWD")) {
      Serial.println("connected");
            display.clearDisplay();
            display.setTextColor(WHITE);
            display.setTextSize(1);
            display.setCursor(0,0);
            display.print("GARAGE DOOR AUTO MQTT                                   LORA OK                                   WiFi OK                                   MQTT OK");
            display.display();
            // Subscribe
            client.subscribe("esp32/output");

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}




void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  if (String(topic) == "esp32/output") {
    
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
      digitalWrite(ledPin, LOW);           //activate Garagedoor UHF sender
      delay(300);                       // wait for a second
      digitalWrite(LED_BUILTIN, LOW);// turn the LED off by making the voltage LOW
      digitalWrite(ledPin, HIGH);     //deactivate Garagedoor UHF sender
      delay(200);          
    }
    else if(messageTemp == "off"){
      Serial.println("off");
      digitalWrite(ledPin, HIGH);
    }
     display.clearDisplay();
     display.setTextColor(WHITE);
     display.setTextSize(1);
     display.setCursor(0,0);
     display.print("GARAGE DOOR OPENED   VIA MQTT");
     display.display();
     long now = millis();
     lastMsg = now;
  }
   long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    display.clearDisplay();
    display.display();
  }
}


void loop() {

  //BasicOTA LOOP START

   ArduinoOTA.handle();

  //BasicOTA LOOP END

  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    String message = "";
    while (LoRa.available()) {
      message = message + ((char)LoRa.read());
    }
    String rssi = "\"RSSI\":\"" + String(LoRa.packetRssi()) + "\"";
    String jsonString = message;
    jsonString.replace("xxx", rssi);

    int ii = jsonString.indexOf("Count", 1);
    String count = jsonString.substring(ii + 8, ii + 11);
    counter = count.toInt();
    if (counter - lastCounter == 0) Serial.println("Repetition");
    lastCounter = counter;

    sendAck(message);
    String value1 = jsonString.substring(8, 11);  // Vcc or heighth
    String value2 = jsonString.substring(23, 26); //counter
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    digitalWrite(ledPin, LOW);           //activate Garagedoor UHF sender
    delay(300);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);// turn the LED off by making the voltage LOW
    digitalWrite(ledPin, HIGH);     //deactivate Garagedoor UHF sender
    delay(200);
     display.clearDisplay();
     display.setTextColor(WHITE);
     display.setTextSize(1);
     display.setCursor(0,0);
     display.print("GARAGE DOOR OPENED   VIA CAR PROXIMITY");
     display.display();
     long now = millis();
     lastMsg = now;
  }
  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    display.clearDisplay();
    display.display();
  }
}


void sendAck(String message) {
  int check = 0;
  for (int i = 0; i < message.length(); i++) {
    check += message[i];
  }
  // Serial.print("/// ");
  LoRa.beginPacket();
  LoRa.print(String(check));
  LoRa.endPacket();
  Serial.print(message);
  Serial.print(" ");
  Serial.print("Ack Sent: ");
  Serial.println(check);
}
