#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Servo.h>

///////////////////////180 ไปหน้า 0 ไปหลัง////////////////////////////
Servo body;
Servo leftFront;
Servo rightFront;
Servo leftBack;
Servo rightBack;

//setting
int statusmach = 0;
boolean connectwifi = true;
boolean autocontrol = false;
boolean linenotify = false;

char prevIRtext[20];
char IRtext[20];

int walkingdelay = 1;
int servoLegTarget[4] = {90, 90, 90, 90};
int bodyangle = 90;

//barking
int melody1[] = {196, 196, 31};
int noteDurations1[] = {8, 8, 4};
int melody2[] = {31, 196, 104, 196, 104, 196, 31};
int noteDurations2[] = {2, 8, 16, 8, 16, 8, 4};
int melody3[] = {31, 196};
int noteDurations3[] = {1, 8};

//----------------INFRARED PART----------------
const uint8_t IR = A0;
const uint8_t speaker = D2;

//-----------INTERNET CONNECTION PART-----------
const char* ssid = "kkkkk";
const char* password = "999999998";
const char* mqtt_server = "broker.netpie.io";
const int mqtt_port = 1883;
const char* mqtt_Client = "4046bf10-81a3-4a9d-a7f8-9ecec5451809";
const char* mqtt_username = "zmuKrFEDiZCw7JbmM5qaxyafs6AD4eQt";
const char* mqtt_password = "*10MmBzNpOF5R3k9Ll49zs73gzoLKrFK";

WiFiClient espClient;
PubSubClient client(espClient);
char msg[100];

void internet_connection(){
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
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection…");
    if (client.connect(mqtt_Client, mqtt_username, mqtt_password)) {
      Serial.println("connected");
      client.subscribe("@msg/statusmach");
      client.subscribe("@msg/autocontrol");
      client.subscribe("@msg/linenotify");
      client.subscribe("@msg/barking");
      client.subscribe("@msg/control");
      client.publish("@shadow/data/update", "{\"data\" : {\"statusmach\" : \"standing\"}}");
    }
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println("try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  String message;
  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];
  }
  Serial.println(message);

  if(String(topic) == "@msg/statusmach") {
    if (message == "standing"){
      client.publish("@shadow/data/update", "{\"data\" : {\"statusmach\" : \"standing\"}}");
      Serial.println("standing");
      turn(90);
      statusmach = 0;
    }
    else if (message == "sitting") {
      Serial.println("sitting");
      client.publish("@shadow/data/update", "{\"data\" : {\"statusmach\" : \"sitting\"}}");
      statusmach = 1;
    }
    else if (message == "walking") {
      Serial.println("walking");
      client.publish("@shadow/data/update", "{\"data\" : {\"statusmach\" : \"walking\"}}");
      statusmach = 2;
    }
  }
  else if(String(topic) == "@msg/autocontrol") {
    if (message == "on" && !autocontrol){
      client.publish("@shadow/data/update", "{\"data\" : {\"autocontrol\" : true}}");
      Serial.println("Toggle on AutoControl Mode");
      autocontrol = true;
    }
    else if(message == "off" && autocontrol){
      client.publish("@shadow/data/update", "{\"data\" : {\"autocontrol\" : false}}");
      Serial.println("Toggle off AutoControl Mode");
      client.publish("@shadow/data/update", "{\"data\" : {\"statusmach\" : \"sitting\"}}");
      autocontrol = false;
    }
  }
  else if(String(topic) == "@msg/linenotify") {
    if (message == "on" && !linenotify){
      client.publish("@shadow/data/update", "{\"data\" : {\"notify\" : true}}");
      Serial.println("Toggle on Line Notify");
      linenotify = true;
    }
    else if(message == "off" && linenotify){
      client.publish("@shadow/data/update", "{\"data\" : {\"notify\" : false}}");
      Serial.println("Toggle off Line Notify");
      linenotify = false;
    }
  }
  else if(String(topic) == "@msg/barking") {
    Serial.println("barking");
    barking();
  }
  else if(String(topic) == "@msg/control") {
    if (message == "left"){
      turn(45);
      Serial.println("turn left");
    }
    else if(message == "straight"){
      turn(90);
      Serial.println("turn straight");
    }
    else if(message == "right"){
      turn(145);
      Serial.println("turn right");
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////

void readDistance(){
  int dist = analogRead(IR);
  dist = map(dist, 50, 500, 255, 0);
  dist = constrain(dist, 0, 255);

  int divisor = map(dist, 0, 255, 10, 3);

  if(dist <= 120){                              //14 cm
    strcpy (IRtext, "Warning");
  }
  else{
    dist = dist/divisor;
    itoa(dist, IRtext, 10);
  }
  Serial.println(IRtext);
}

void legMove(char* leftright, char* frontback, int angle)
{
  if(leftright == "l" && frontback == "f"){
    leftFront.write(map(angle, 0, 180, 180, 0));
  }
  else if(leftright == "r" && frontback == "f"){
    rightFront.write(angle);
  }
  else if(leftright == "l" && frontback == "b"){
    leftBack.write(map(angle, 0, 180, 180, 0));
  }
  else if(leftright == "r" && frontback == "b"){
    rightBack.write(angle);
  }
}

/////////////////////////////////////////////////////////////////////////////
void setup()
{
  Serial.begin(115200);
  
  //SERVO PART
  body.attach(16, 500, 2500); //D0
  leftFront.attach(14, 500, 2500); //D5
  rightFront.attach(12, 500, 2500); //D6
  leftBack.attach(13, 500, 2500); //D7
  rightBack.attach(15, 500, 2500); //D8

  // INTERNET PART
  if(connectwifi){
    internet_connection();
  }

  //PIN AND VARIABLE SETTING
  pinMode(IR, INPUT);
  pinMode(speaker, OUTPUT);
}

//Leg State Function
boolean isState(int lf, int rf, int lb, int rb){
  int statearray[4] = {lf, rf, lb, rb};
  for(int i=0; i<4; i++){
    if(servoLegTarget[i] != statearray[i]){
      return false;
    }
  }
  return true;
}

void changeTarget(int lf, int rf, int lb, int rb){
  servoLegTarget[0] = lf;
  servoLegTarget[1] = rf;
  servoLegTarget[2] = lb;
  servoLegTarget[3] = rb;
}

void walking(){
  if(isState(90, 90, 90, 90)){
    for(int i = 90; i <= 130; i++){
      legMove("l", "f", i);
      delay(walkingdelay);
    }
    changeTarget(130, 90, 90, 90);
  }
  else if(isState(130, 90, 90, 90)){
    for(int i = 90; i <= 130; i++){
      legMove("r", "b", i);
      delay(walkingdelay);
    }
    changeTarget(130, 90, 90, 130);
  }
  else if(isState(130, 90, 90, 130)){
    for(int i = 90; i <= 130; i++){
      legMove("r", "f", i);
      legMove("l", "f", 220-i);
      delay(walkingdelay);
    }
    changeTarget(90, 130, 90, 130);
  }
  else if(isState(90, 130, 90, 130)){
    for(int i = 90; i <= 130; i++){
      legMove("l", "b", i);
      legMove("r", "b", 220-i);
      delay(walkingdelay);
    }
    changeTarget(90, 130, 130, 90);
  }
  else if(isState(90, 130, 130, 90)){
    for(int i = 90; i <= 130; i++){
      legMove("l", "f", i);
      legMove("r", "f", 220-i);
      delay(walkingdelay);
    }
    changeTarget(130, 90, 130, 90);
  }
  else if(isState(130, 90, 130, 90)){
    for(int i = 90; i <= 130; i++){
      legMove("r", "b", i);
      legMove("l", "b", 220-i);
      delay(walkingdelay);
    }
    changeTarget(130, 90, 90, 130);
  }
  else{
    legMove("l", "f", 90);
    legMove("r", "f", 90);
    legMove("l", "b", 90);
    legMove("r", "b", 90);
    changeTarget(90, 90, 90, 90);
  }
}

void sitting(){
  if(!isState(90, 90, 170, 170)){
    legMove("l", "f", 90);
    legMove("r", "f", 90);
    legMove("l", "b", 90);
    legMove("r", "b", 90);
    for(int i = 90; i < 170; i++){
      legMove("l", "b", i);
      legMove("r", "b", i);
    }
    for(int i = 90; i > 50; i--){
      legMove("l", "f", i);
      legMove("r", "f", i);
    }
    changeTarget(90, 90, 170, 170);
  }
}

void barking(){
  int randnum = random(3);
  if(randnum == 0){
    for (int thisNote = 0; thisNote < 3; thisNote++) {
      int noteDuration = 1000/noteDurations1[thisNote];
      tone(speaker, melody1[thisNote], noteDuration); 
      delay(noteDuration * 4 / 3);
    }
  }
  else if(randnum == 1){
    for (int thisNote = 0; thisNote < 7; thisNote++) {
      int noteDuration = 1000/noteDurations2[thisNote];
      tone(speaker, melody2[thisNote], noteDuration); 
      delay(noteDuration * 4 / 3);
    }
  }
  else{
    for (int thisNote = 0; thisNote < 2; thisNote++) {
      int noteDuration = 1000/noteDurations3[thisNote];
      tone(speaker, melody3[thisNote], noteDuration); 
      delay(noteDuration * 4 / 3);
    }
  }
}

void turn(int legangle){
  body.write(legangle);
  bodyangle = legangle;
}

/////////////////////////////////////////////////////////////////////////////
int infraread = 0;
void loop() {
  if(connectwifi){
    if (!client.connected()) {
      reconnect();
    }
    client.loop();
    long now = millis();
  }
  
  //serial monitor control
  if (Serial.available() > 0){
    char data = Serial.read();
    if (data == '0'){
      statusmach = 0;
      Serial.println("standing");
    }
    else if (data == '1'){
      statusmach = 1;
      Serial.println("sitting");
    }
    else if (data == '2'){
      statusmach = 2;
      Serial.println("walking");
    }
  }

  //auto control mode
  if(autocontrol){
    if(strcmp(IRtext, "Warning") == 0){
      sitting();
      client.publish("@shadow/data/update", "{\"data\" : {\"statusmach\" : \"sitting\"}}");
      delay(2000);
    }else{
      walking();
      client.publish("@shadow/data/update", "{\"data\" : {\"statusmach\" : \"walking\"}}");
    }
  }else{
    //leg control
    if(statusmach == 0){                //Standing
      legMove("l", "f", 90);
      legMove("r", "f", 90);
      legMove("l", "b", 90);
      legMove("r", "b", 90);
      turn(90);
      changeTarget(90, 90, 90, 90);
    }
    else if(statusmach == 1){           //Sitting
      sitting();
    }
    else if(statusmach == 2){           //Walking
      walking();
    } 
  }

  //infrared
  if(infraread > 5 || statusmach == 2){
    readDistance();
    infraread = 0;
    if(prevIRtext != IRtext){
      String data = "{\"data\" : {\"distance\" : \"" + String(IRtext) + "\"}}";
      data.toCharArray(msg, (data.length() + 1));
      client.publish("@shadow/data/update", msg);
      strcpy (prevIRtext, IRtext);
    }
  }else{
    infraread++;
  }
  
  delay(100);
}
