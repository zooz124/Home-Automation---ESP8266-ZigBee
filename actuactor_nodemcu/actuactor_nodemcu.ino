#include <PubSubClient.h>
#include <ESP8266WiFi.h>

const char* mqtt_server = "10.10.66.200";

//static const uint8_t D0   = 16; // ACS_1
//static const uint8_t D1   = 5;  // temp
//static const uint8_t D2   = 4;  // ACS_2
//static const uint8_t D4   = 2;  // relay 1 - plug
//static const uint8_t D8   = 15; // relay 4 - light
//static const uint8_t D7; 		  // relay 7 - simulated


WiFiClient espClient;
PubSubClient client(espClient);

char state[40];
char topic_s[40];
char msgbuf[50];
float  power, temp = 0;
unsigned long timer=millis();
bool relayState1 = HIGH;
bool relayState2 = HIGH;
bool relayState = HIGH;

void setup_wifi()
{ 
  delay(10);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected"); 
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup()
{
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D8, OUTPUT);
  pinMode(D7, OUTPUT);
  digitalWrite(D0, LOW);
  digitalWrite(D1, LOW);
  digitalWrite(D2, LOW);
  digitalWrite(D4, HIGH);
  digitalWrite(D8, HIGH);
  digitalWrite(D7, HIGH);
  Serial.begin(9600);   //USB as debug, external power
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void callback(char* topic, byte* payload, unsigned int length) 
{
  memset(state, 0, strlen(state));
  memset(topic_s, 0, strlen(topic_s));
  for (int i = 0; i < length; i++) {
    state[i] = (char)payload[i];
  }
  for (int i = 0; i < strlen(topic); i++)
     topic_s[i] = topic[i]; 
//  client.publish("/debug/node1", topic_s);
//  client.publish("/debug/node1", state);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect("ESP8266Client_1")) {
      // Once connected, publish an announcement...
      client.publish("/casa/P0/plug", "I'm alive!");
      client.publish("/casa/P0/plug/power", "I'm alive!");
      client.publish("/casa/P0/plug/temp", "I'm alive!");
      client.publish("/casa/P0/light", "I'm alive!");
      client.publish("/casa/P0/light/power", "I'm alive!");
      client.publish("/casa/P0/heating", "I'm alive!");
      // ... and resubscribe
      client.subscribe("/casa/P0/plug");
      client.subscribe("/casa/P0/plug/power");
      client.subscribe("/casa/P0/plug/temp");
      client.subscribe("/casa/P0/light");
      client.subscribe("/casa/P0/light/power");
      client.subscribe("/casa/P0/heating");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

int get_ADC_peak()
{
  int value;
  int Vmax = 0;
  long tiempo=millis();
  while(millis()-tiempo<20)  // 1 cycle at 50Hz to get the peak
  {
    value = analogRead(A0);
    if(value>Vmax)Vmax=value;
  }
  return Vmax;
}

float get_RMS_Current()
{
  int VmaxADC = get_ADC_peak();
  float Volt = VmaxADC * 3.22 / 1000;  // measured with potentiometer
  if (Volt < 2.5)
    return 0;
  float Current = 0.707 * (Volt - 2.49) / 0.185;  // Vef
  return Current;
}

void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (strcmp(state, "ON") == 0 && relayState == HIGH && strcmp(topic_s, "/casa/P0/heating") == 0) //this mqtt message is send by the client
  {
    Serial.println("turn ON");
    digitalWrite(D7, LOW);
    relayState = LOW;
  }
  if (strcmp(state, "OFF") == 0 && relayState == LOW && strcmp(topic_s, "/casa/P0/heating") == 0)
  {
    Serial.println("turn OFF");
    digitalWrite(D7, HIGH); 
    relayState = HIGH;
  }

  if (strcmp(state, "ON") == 0 && relayState1 == HIGH && strcmp(topic_s, "/casa/P0/plug") == 0) //this mqtt message is send by the client
  {
    client.publish("/debug/node1", "turn ON plug");
    digitalWrite(D4, LOW);
    relayState1 = LOW;
  }
  if (strcmp(state, "OFF") == 0 && relayState1 == LOW && strcmp(topic_s, "/casa/P0/plug") == 0)
  {
  //  client.publish("/debug/node1", "turn OFF plug");
    digitalWrite(D4, HIGH); 
    relayState1 = HIGH;
  }

  if (strcmp(state, "ON") == 0 && relayState2 == HIGH && strcmp(topic_s, "/casa/P0/light") == 0) //this mqtt message is send by the client
  {
   // client.publish("/debug/node1", "turn ON light");
    digitalWrite(D8, LOW);
    relayState2 = LOW;
  }
  if (strcmp(state, "OFF") == 0 && relayState2 == LOW && strcmp(topic_s, "/casa/P0/light") == 0)
  {
    //client.publish("/debug/node1", "turn OFF light");
    digitalWrite(D8, HIGH); 
    relayState2 = HIGH;
  }


  if (millis()-timer>=5000)  // 5 sec
  {
    digitalWrite(D2, HIGH);
    float current = get_RMS_Current();
    digitalWrite(D2, LOW); 
    power = current*220;

    client.publish("/casa/P0/plug/power", dtostrf(power, 0, 2, msgbuf));

    digitalWrite(D1, HIGH);
    temp = analogRead(A0);
    digitalWrite(D1, LOW);
    temp = (5.0 * temp * 100.0)/1024.0;  //convert the analog data to temperature
    client.publish("/casa/P0/plug/temp", dtostrf(temp, 0, 2, msgbuf));

    digitalWrite(D0, HIGH);
    current = get_RMS_Current();
    digitalWrite(D0, LOW);
    power = current*220;
    if (power == 0)
        client.publish("/casa/P0/light/power", "0");
    else   
        client.publish("/casa/P0/light/power", dtostrf(power, 0, 2, msgbuf));

    timer=millis();
  }
}
