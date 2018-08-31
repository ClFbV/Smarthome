//#include <PubSubClient.h>

#include "ESP8266WiFi.h"
#include <EEPROM.h>
#include <ESP8266Ping.h>
#include <ESP8266WebServer.h>
#include "./DNSServer.h"
#include "MQTT.h"


const char* mqtt_server = "clfbv.cf";
const char* user= "NodeMCU";
const char* password= "SmartHome";
char* ssid;
char* passw;
String ssidx;
IPAddress    apIP(10, 10, 10, 1);

WiFiClient espClient;
MQTTClient client;
//Creating server...
ESP8266WebServer server(80);
DNSServer        dnsServer;
String responseHTML = "<!DOCTYPE html><html><head><title>Sensor 1</title></head><body>"
                      "<h1>Welcome to SmartHome!</h1><p>Insert your SSID and password"
                      "here to configure this sensor.</p>"
                      "<form action='http://10.10.10.1/submit' method='POST'>"
                      "SSID:<br><input type='text' name='ssid' value='SSID'><br>"
                      "Password:<br><input type='text' name='password' value='password'><br><br>"
                      "<input type='submit' value='Submit'></body></html>"
                      "</form>";

long lastMsg = 0;
char msg[50];
int value = 0;
int wifitry = 0;
int ssidpswins = 0;
// int ssidlen2;
// int passwlen;

void(* Reboot)(void) = 0;

void handleRoot() {
  server.send(200, "text/plain", responseHTML);
}

int handleSubmit() {
  if (server.args() > 0 ) {
    for ( uint8_t i = 0; i < server.args(); i++ ) {
      if (server.argName(i) == "ssid") {
        String ssidx = server.arg(i);
        Serial.println(ssidx);
        char ssid[ssidx.length()];
        ssidx.toCharArray(ssid, ssidx.length());
        // for (int j = 0; j < ssidx.length(); j++) {
        //   EEPROM.write(j + 2, ssid[j]);
        //   EEPROM.commit();
        // }
        // EEPROM.put(0, ssidx.length() + 2);
        // EEPROM.commit();
      }
      else if (server.argName(i) == "password") {
        String passwx = server.arg(i);
        Serial.println(passwx);
        char passw[passwx.length()];
        passwx.toCharArray(passw, passwx.length());
        // for (int k = ssidx.length() + 3; k < (passwx.length() + ssidx.length() + 3); k++) {
        //   EEPROM.put(k, passw[k - (ssidx.length() + 3)]);
        //   EEPROM.commit();
        // }
        // EEPROM.put(1, passwx.length());
        // EEPROM.commit();
        return 1;
      }
    }
  }
}

void WiFiuserpass() {
  Serial.print("Setting soft-AP ... ");
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  boolean result = WiFi.softAP("Sensor 1", "smarthome");
  if(result == true)
  {
    Serial.println("Ready");
  }
  else
  {
    Serial.println("Failed!");
  }
  server.on("/", handleRoot);
  server.on("/submit", handleSubmit);
  //Redirect all requests to our IP
  dnsServer.start(53, "*", apIP);
  //Start server
  server.begin();
  server.onNotFound([]() {
    server.send(200, "text/html", responseHTML);
  });
  Serial.println("Server Started");
  while(1) {
    dnsServer.processNextRequest();
    server.handleClient();
    if(handleSubmit() == 1) {
      WiFi.softAPdisconnect(true);
      ssidpswins = 1;
      break;
    }
  }
  setup_wifi();
}

void setup_wifi() {
  delay(10);
  if (ssidpswins == 0) {
    WiFiuserpass();
  }
  // EEPROM.get(0, ssidlen2);
  // EEPROM.get(1, passwlen);
  // if (ssidlen2!=NULL || ssid!=NULL) {
  //   for (int x = 2; x < ssidlen2 + 1; x++) {
  //     ssid[x - 2] = EEPROM.read(x);
  //   }
  //   for (int y = ssidlen2 + 1; y < (passwlen + ssidlen2 + 1); y++) {
  //     passw[y - (ssidlen2 + 1)] = EEPROM.read(y);
  //   }
  // }
  // else {
  //   WiFiuserpass();
  // }
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, passw);

  while (WiFi.status() != WL_CONNECTED) {
    int wifitry = wifitry + 1;
    delay(1000);
    Serial.print(".");
    if (wifitry > 14) {
      Reboot();
    }
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);

  // Switch on the LED if an 1 was received as first character
  if (payload[0] == '1') {
    digitalWrite(D4, HIGH);
  } else if (payload[0] == '0'){
    digitalWrite(D4, LOW);
  }
  else {
      client.publish("EthErr", "Send 0 or 1 - UC", 0, 2);
  }

}

void mqttconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), user, password)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("EthErr", "Connected!", 0, 2);
      // ... and resubscribe
      client.subscribe("ToSens", 2);
    } else {
        if ((!Ping.ping("clfbv.cf")) && (Ping.ping("google.com"))) {
          Serial.println("Internet connection working, server down");
          //Change mqtt_server
        }
        else if ((!Ping.ping("clfbv.cf")) && (!Ping.ping("google.com"))) {
          Serial.println("Internet connection not working");
          setup_wifi();
        }
      Serial.print("failed...");
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(D4, OUTPUT);     // Initialize the D4 pin as an output
  pinMode(D3, OUTPUT);
  pinMode(A0, INPUT);
  EEPROM.begin(512);
  Serial.begin(115200);
  digitalWrite(D3, LOW);
  setup_wifi();
  client.begin(mqtt_server, espClient);
  client.onMessage(callback);
  mqttconnect();
}

void loop() {
  if (!client.connected()) {
    mqttconnect();
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 2000) {
     lastMsg = now;
     int eth = analogRead(A0);
     snprintf (msg, 75, "Ethanol = %ld", eth);
     Serial.print("Publish message: ");
     Serial.println(msg);
     client.publish("EthSens", msg, 0, 2);
   }
}
