/*
  Name:        BoilerBot.ino
  Created:     26/03/2021
  Author:      Tolentino Cotesta <cotestatnt@yahoo.com>
  Description: a simple example that do:
             1) parse incoming messages
             2) if "BOILER ON" message is received, turn on the D0 pin
             3) if "BOILER OFF" message is received, turn off the D0 pin
             4) otherwise, reply to sender with a welcome message

*/
#include "credentials.h"

/* 
  Set true if you want use external library for SSL connection instead ESP32@WiFiClientSecure 
  For example https://github.com/OPEnSLab-OSU/SSLClient/ is very efficient BearSSL library.
  You can use AsyncTelegram2 even with other MCUs or transport layer (ex. Ethernet)
  With SSLClient, be sure "certificates.h" file is present in sketch folder
*/ 
#define USE_CLIENTSSL false

#include <AsyncTelegram2.h>
// Timezone definition
#include <time.h>
#define MYTZ "CET-1CEST,M3.5.0,M10.5.0/3"

#ifdef ESP8266
  #include <ESP8266WiFi.h>
  BearSSL::WiFiClientSecure client;
  BearSSL::Session   session;
  BearSSL::X509List  certificate(telegram_cert);
  
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WiFiClient.h>
  #if USE_CLIENTSSL
    #include <SSLClient.h>  
    #include "tg_certificate.h"
    WiFiClient base_client;
    SSLClient client(base_client, TAs, (size_t)TAs_NUM, A0, 1, SSLClient::SSL_ERROR);
  #else
    #include <WiFiClientSecure.h>
    WiFiClientSecure client;    
  #endif
#endif

#include <ESP8266HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

AsyncTelegram2 myBot(client);
const char*   ssid               = WLAN_SSID;                   // SSID WiFi network
const char*   pass               = WLAN_PASS;                   // Password  WiFi network
const char*   token              = TELEGRAM_TOKEN;              // Telegram token
const int64_t contactUser        = TELEGRAM_CONTACT_USER;       // Telegram contact user
const char    permittedChatIds[] = TELEGRAM_PERMITTED_CHAT_IDS; // Telegram permitted chat IDs
const char*   noipUsername       = NOIP_USERNAME;               // No-IP username
const char*   noipPassword       = NOIP_PASSWORD;               // No-IP password
const char*   noipDomain         = NOIP_DOMAIN;                 // No-IP domain

WiFiUDP ntpUdp;
NTPClient timeClient(ntpUdp, "pool.ntp.org");

int boilerState = HIGH;
int lastButtonState;
int currentButtonState;

bool noIPUpdated = false;

time_t currentTime = 0;
time_t timerEnd = 0;

// const uint8_t LED = LED_BUILTIN;

String boilerStateStr() {
  if(boilerState) {
    return "wylaczony";
  } else {
    return "zalaczony";
  }
}

void enableTimer() {
  if(currentTime > 0) {
    timerEnd = currentTime + (30 * 60);
  } else {
    myBot.sendTo(contactUser, "Blad uruchamiania timera - brak kontaktu z serwerem NTP!");
  }
}

String timerStateStr() {
  if(timerEnd > 0) {
    return String("zalaczony (" + String((timerEnd - currentTime) / 60) + " min)");
  } else {
    return String("wylaczony");
  }
}

void setup() {
  // initialize the Serial
  Serial.begin(9600);
  Serial.println("\nStarting TelegramBot...");

  pinMode(D1, INPUT);
  currentButtonState = digitalRead(D1);

  // set the pin connected to the D0 pin to act as output pin
  pinMode(D0, OUTPUT);
  digitalWrite(D0, boilerState); // turn off the D0 pin (inverted logic!)

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  delay(500);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(500);
  }

#ifdef ESP8266
  // Sync time with NTP, to check properly Telegram certificate
  configTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  //Set certficate, session and some other base client properies
  client.setSession(&session);
  client.setTrustAnchors(&certificate);
  client.setBufferSizes(1024, 1024);
#elif defined(ESP32)
  // Sync time with NTP
  configTzTime(MYTZ, "time.google.com", "time.windows.com", "pool.ntp.org");
  #if USE_CLIENTSSL == false
    client.setCACert(telegram_cert);
  #endif
#endif

  // Set the Telegram bot properies
  myBot.setUpdateTime(1000);
  myBot.setTelegramToken(token);

  // Check if all things are ok
  Serial.print("\nTest Telegram connection... ");
  myBot.begin() ? Serial.println("OK") : Serial.println("NOK");

  Serial.print("Bot name: @");
  Serial.println(myBot.getBotName());

  timeClient.begin();
}

void loop() {
  timeClient.update();

  currentTime = timeClient.getEpochTime();

  lastButtonState = currentButtonState;
  currentButtonState = digitalRead(D1);

  if(lastButtonState == HIGH && currentButtonState == LOW) {
    boilerState = !boilerState;
    digitalWrite(D0, boilerState);

    if(!boilerState) {
      enableTimer();
    } else {
      timerEnd = 0;
    }

    String message;
    message += "Boiler zostal recznie " + boilerStateStr();
    myBot.sendTo(contactUser, message);
  }

  // local variable to store telegram message data
  TBMessage msg;
  
  // if there is an incoming message...
  if (myBot.getNewMessage(msg)) {
    bool currentChatIsPermitted = false;
    if (permittedChatIds != "") {
      char permittedChatIdsLocal[sizeof(permittedChatIds)];
      strcpy(permittedChatIdsLocal, permittedChatIds);
      char *permittedChatIdsPointer = strtok(permittedChatIdsLocal, ",");
      while (permittedChatIdsPointer != NULL && ! currentChatIsPermitted) {
        if(String(permittedChatIdsPointer) == String(msg.chatId)) {
          currentChatIsPermitted = true;
        }
        permittedChatIdsPointer = strtok(NULL, ",");
      }
    }
    if (! currentChatIsPermitted) {
      return;
    }
    String msgText = msg.text;

    if (msgText.equals("/boiler_on") || msgText.equals("/boiler_on@BoilerSwitcherBot")) {         // if the received message is "BOILER ON"...
      boilerState = LOW;
      digitalWrite(D0, boilerState);                                                              // turn on the D0 pin (inverted logic!)
      enableTimer();
      myBot.sendMessage(msg, "Boiler jest " + boilerStateStr());                                  // notify the sender
    }
    else if (msgText.equals("/boiler_off") || msgText.equals("/boiler_off@BoilerSwitcherBot")) {  // if the received message is "BOILER OFF"...
      boilerState = HIGH;
      digitalWrite(D0, boilerState);                                                              // turn off the D0 pin (inverted logic!)
      timerEnd = 0;
      myBot.sendMessage(msg, "Boiler jest " + boilerStateStr());                                  // notify the sender
    }
    else if (msgText.equals("@BoilerSwitcherBot") || msgText.equals("/start")) {                  // otherwise...
      // generate the message for the sender
      String reply;
      reply = "Dzien dobry " ;
      reply += msg.sender.firstName;
      reply += ".\nStan boilera: " + boilerStateStr();
      reply += ".\nTimer: " + timerStateStr();
      reply += ".\nUzyj polecenia /boiler_on lub /boiler_off ";
      myBot.sendMessage(msg, reply);                                                              // and send it
    }
  }

  if(! noIPUpdated || millis() % (10 * 60 * 1000) == 0) {
    Serial.print("\nUpdating No-IP domain... ");
    WiFiClient client;
    HTTPClient http;

    String noipUrl = "http://" + String(noipUsername) + ":" + String(noipPassword) + "@dynupdate.no-ip.com/nic/update?hostname=" + String(noipDomain);

    http.begin(client, noipUrl);
    http.setUserAgent("wtc.lv TelegramBotESP/1.0 contact@wtc.lv");

    int httpResponse = http.GET();

    if(httpResponse == 200) {
      Serial.print("OK");
      Serial.println("\n" + http.getString());
    } else {
      Serial.print("ERR");
    }

    noIPUpdated = true;
  }

  if(currentTime > 0 && timerEnd > 0 && timerEnd < currentTime) {
    timerEnd = 0;
    boilerState = HIGH;
    digitalWrite(D0, boilerState);   
    myBot.sendTo(contactUser, "Boiler zostal wylaczony przez timer");
  }
}
