#include <Arduino.h>
#include <ESP8266WiFi.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
#include <PubSubClient.h>
#pragma GCC diagnostic pop

#include <IRremoteESP8266.h>
#include <IRsend.h>

#include "config.inc"

static const char* mqtt_server = "bemfa.com"; // MQTT Server
static const int mqtt_server_port = 9501; // MQTT Server Port

// static bool status_initialized = false;

static WiFiClient espClient;
static PubSubClient client(espClient);

static IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// static bool wsState = true; // assume the light is on when start up, for forcing switching off.
static bool wsState = false;

static void switch_light(bool isOn) {
    if (wsState == isOn) {
        return;
    }

    wsState = isOn;

    digitalWrite(LED_IND, LED_VALUE(wsState));

    // Send 2 times
    irsend.sendRaw(wsState ? rawDataOn : rawDataOff, rawDataLen, sendRawInHz);
    delay(100);
    irsend.sendRaw(wsState ? rawDataOn : rawDataOff, rawDataLen, sendRawInHz);

    // confirming
    client.publish(topic, wsState ? "on" : "off", true);
}

static void setup_wifi() {
    delay(10);

    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.print("WiFi connected. IP address: ");
    Serial.println(WiFi.localIP());
}

static void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");

    String msg = "";
    for (unsigned int i = 0; i < length; i++) {
        msg += (char)payload[i];
    }
    Serial.print(msg);
    Serial.println();

    if (msg.startsWith("on") || msg.startsWith("on#")) {
        switch_light(true);
    } else if (msg.startsWith("off") || msg.startsWith("off#")) {
        switch_light(false);
    }
    msg = "";
}

static void reconnect() {
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect(mqtt_id)) {
            Serial.println("connected");

            client.subscribe(topic, 1);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 1 seconds before retrying
            delay(1000);
        }
    }

    // if (!status_initialized) {
    //     Serial.println("Update the topic...");
    //     client.publish(topic, "off", true); // update the status
    //     status_initialized = true;
    // }
}

void setup() {
    Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY);

    // init IR
    irsend.begin();

    // Init LED Indicator
    pinMode(LED_IND, OUTPUT);
    digitalWrite(LED_IND, LED_VALUE(wsState));

    // Init WiFi and MQTT
    setup_wifi();
    client.setServer(mqtt_server, mqtt_server_port);
    client.setCallback(mqtt_callback);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}
