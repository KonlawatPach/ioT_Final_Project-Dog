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
boolean connectwifi = false;
boolean autocontrol = false;
boolean linenotify = false;
char IRtext[20];

int walkingdelay = 1;
int servoLegTarget[4] = {90, 90, 90, 90};

//----------------INFRARED PART----------------
const uint8_t IR = A0;

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
      Serial.println("standing");
      statusmach = 0;
//      client.publish("@shadow/data/update", "{\"data\" : {\"led\" : \"on\"}}");
    }
    else if (message == "sitting") {
      Serial.println("sitting");
//      client.publish("@shadow/data/update", "{\"data\" : {\"led\" : \"off\"}}");
      statusmach = 1;
    }
    else if (message == "walking") {
      Serial.println("walking");
      statusmach = 2;
    }
  }
  else if(String(topic) == "@msg/autocontrol") {
    if (message == "on" && !autocontrol){
      Serial.println("Toggle on AutoControl Mode");
      autocontrol = true;
    }
    else if(message == "off" && autocontrol){
      Serial.println("Toggle off AutoControl Mode");
      autocontrol = false;
    }
  }
  else if(String(topic) == "@msg/linenotify") {
    if (message == "on" && !linenotify){
      Serial.println("Toggle on Line Notify");
      linenotify = true;
    }
    else if(message == "off" && linenotify){
      Serial.println("Toggle off Line Notify");
      linenotify = false;
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
//  body.attach(16, 500, 2500); //D1
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
  if(!isState(90, 90, 180, 180)){
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
    //auto control code
  }
  
  //leg control
  if(statusmach == 0){                //Standing
    legMove("l", "f", 90);
    legMove("r", "f", 90);
    legMove("l", "b", 90);
    legMove("r", "b", 90);
    changeTarget(90, 90, 90, 90);
  }
  else if(statusmach == 1){           //Sitting
    sitting();
  }
  else if(statusmach == 2){           //Walking
    walking();
  }

  //infrared
  if(infraread > 5 || statusmach == 2){
    readDistance();
    infraread = 0;
  }else{
    infraread++;
  }
  delay(100);
}
