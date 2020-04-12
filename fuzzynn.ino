#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <ThingSpeak.h>

float setpoint, lasterr;
float suhu = 0, error = 0, edot = 0, out,level;
float flow,umf, uP, uN, uZ, uZ1, uP1, uN1, uC, uS, uL, Maks, umx, keluaran, N_error[3], N_edot[3], kerror, kedot, Min[9];
// flow , level , valve , relay
int l = 2, d = 4, a = 5, r = 0, relay, period;
int t1,sigma,t2,Flow_read = 0, valve;
unsigned long timenow, lasttime = 0;
const int e = 3; // temp
OneWire oneWire(e);
DallasTemperature sensors(&oneWire);
char ssid[] = "XLGO-DB33";             // your network SSID (name) 
char pass[] = "brilian2001";         // your network password 
WiFiClient  client;

int number = 0;

void mf(int nomor, float Nilai, float A, float B, float C) {
  switch (nomor) {
    case 1:
      if ((Nilai <= B)) umf = 1;
      if ((Nilai > B) && (Nilai < C)) umf = (C - Nilai) / (C - B);
      if (Nilai >= C) umf = 0;
      break;
    case 2:
      if ((Nilai <= A) || (Nilai >= C)) umf = 0;
      if ((Nilai > A) && (Nilai < B)) umf = (Nilai - A) / (B - A);
      if ((Nilai > B) && (Nilai < C)) umf = (C - Nilai) / (C - B);
      if (Nilai == B) umf = 1;
      break;
    case 3:
      if (Nilai <= A) umf = 0;
      if ((Nilai > A) && (Nilai < B)) umf = (Nilai - A) / (B - A);
      if (Nilai >= B) umf = 1;
      break;
    case 4:
      if ((Nilai >= A) && (Nilai <= B)) umf = 1;
      if ((Nilai > B) && (Nilai < C)) umf = (C - Nilai) / (C - B);
      if (Nilai >= C) umf = 0;
      break;
  }
}

void fuzzificationinput(){
  // error triangular mf
  umf = 0;
  mf(3,error,10,20,0); //P
  uP = umf;
  mf(2,error,-4,0,4); //Z
  uZ = umf;
  mf(1,error,0,-7,0); //N
  uN = umf;
  // edot triangular mf
  umf = 0;
  mf(3,edot,4,20,0); //P1
  uP1 = umf;
  mf(2,edot,-5,0,5); //Z1
  uZ1 = umf;
  mf(1,edot,0,-8,0); //N1
  uN1 = umf;
}

int fs[3][3] = { // Fuzzy sets rule
    {1,1,2},
    {1,1,3},
    {1,2,3}
};

int inferencing(int nomors){
  // Fuzzy Set 3 x 3 rule
  switch (nomors){
    case 1:
    if (Maks <= 35) out = umx*35 ;
    if ((Maks > 35) && (Maks <=70)) out = 70 - umx*35 ;
    case 2:
    if ((Maks <= 135) && (Maks > 60)) out = umx*135 ;
    if ((Maks > 135) && (Maks <= 210)) out = 210 - umx*135 ;
    case 3:
    if ((Maks > 200) && (Maks <= 0)) out = umx*350;
    if ((Maks > 250) && (Maks <= 500)) out = 500 - umx*350;
  }
  return out;
}

void defuzzification(){
  float num = 0, den = 0, coa = 0;
  N_error[3] = {};
  N_edot[3] = {};

  for (int set = 0; set < 9;) {
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 3; j++) {
        float uerror[3] = {uN, uZ, uP};
        N_error[i] = uerror[i];
        float uedot[3] = {uN1, uZ1, uP1};
        N_edot[j] = uedot[j];

        kerror = max(N_error[i], kerror);
        kedot = max(N_edot[j], kerror);

        /* Metode COA (Centre Of Area)*/
        Min[set] = min(N_error[i], N_edot[j]);
        float mem = inferencing(fs[i][j]);
        num  += Min[set] * mem;
        den  += Min[set];
        delay(5);
        set ++;
      }
    }
  }
  coa = num / den; 
  keluaran = coa;
}

void stateon(){
  int itung = 1;
  keluaran =  keluaran * 1000;
  while ( timenow < timenow + keluaran){
   if (itung == 1){ 
    digitalWrite(6, HIGH);
    relay = 1;
    updatecolumn();
    Serial.println("Column Heater on");
   }
   if (timenow % 15000 == 0){
    updatesuhu(); // Update data suhu
    updateflow(); // Update data flow
    updatelevel(); // Update data level
    updatevalve(); // Update data valve
   }
   itung++;   
  }
   digitalWrite(6, LOW);
   relay = 0;
   updatecolumn();
   Serial.println("Column Heater off");
   itung = 0;
}

void updatesuhu(){
  int x = ThingSpeak.writeField(903702, 1, suhu, "5IYGCQRLBI4T9NC8"); // Upload pembacaan temperature ke Cloud
  if(x == 200){
    Serial.println("Temperature update successful.");
  }
  else{
    Serial.println("Problem updating temperature. HTTP error code " + String(x));
  }
}

void updateflow(){
  int xx = ThingSpeak.writeField(903702, 2, flow, "5IYGCQRLBI4T9NC8"); // Upload pembacaan flow ke Cloud
  if(xx == 200){
    Serial.println("Flow rate update successful.");
  }
  else{
    Serial.println("Problem flow rate. HTTP error code " + String(xx));
  }
}

void updatelevel(){
  int xy = ThingSpeak.writeField(903702, 3, level, "5IYGCQRLBI4T9NC8"); // Upload pembacaan Level ke Cloud
  if(xy == 200){
    Serial.println("Level update successful.");
  }
  else{
    Serial.println("Problem updating level. HTTP error code " + String(xy));
  }
}

void updatevalve(){
  int xz = ThingSpeak.writeField(903702, 4, valve, "5IYGCQRLBI4T9NC8"); // Upload pembacaan kondisi valve ke Cloud
  if(xz == 200){
    Serial.println("Valve state update successful.");
  }
  else{
    Serial.println("Problem updating valve state. HTTP error code " + String(xz));
  }
}

void updatecolumn(){
  int xa = ThingSpeak.writeField(903702, 7, relay, "5IYGCQRLBI4T9NC8"); // Upload pembacaan temperature ke Cloud
  if(xa == 200){
    Serial.println("Column status update successful.");
  }
  else{
    Serial.println("Problem updating column status. HTTP error code " + String(xa));
  }
}

void setup() {
  Serial.begin(9600);
  sensors.begin(); // Mulai sensor temperature
  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client);
  pinMode(d, INPUT_PULLUP); //level
  pinMode(l,INPUT); // flow
  pinMode(a, OUTPUT); // valve
  pinMode(r , OUTPUT);
}

void loop() {
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    while(WiFi.status() != WL_CONNECTED){
      WiFi.begin(ssid, pass);
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }
  float flow = (digitalRead(l));
  sigma = Flow_read+t2;
  Flow_read = t2;
  t2 = sigma;
  float lev = sigma / 60; 
  Serial.print(digitalRead(d)); //read level
  Serial.print(digitalRead(l));  //read flow
  Serial.print(sigma/60);
  delay(1);
  if(digitalRead(l) == LOW) 
  {
   // turn solenoid on:
   Serial.print("TANK IS LOW");
   digitalWrite(a, LOW);  //Switch Solenoid on
   valve = 0;             
  } 
  else 
  {
   // turn solenoid off:
   Serial.print("TANK IS FULL");
   digitalWrite(a, HIGH); //Switch Solenoid off
   valve = 1;
  }
  sensors.requestTemperatures();
  int maxx = 1000; // Nilai threshold level maximum
  lasttime = timenow;
  timenow = millis();
  float timediff = timenow - lasttime;
  setpoint = 82; // Setpoint suhu
  suhu = sensors.getTempCByIndex(0); //baca temperature
  Serial.print("Temperature :");
  Serial.print(suhu);
  Serial.println("ºC");
 
  /*if (level < maxx){
    relay = 0;  
    digitalWrite(6, LOW);
    Serial.println("Relay off");
  }
  else {
    relay = 1;
    digitalWrite(6, HIGH);
    Serial.println("Relay on");
  }*/
  updatesuhu(); // Update data suhu
  updateflow(); // Update data flow
  updatelevel(); // Update data level
  updatevalve(); // Update data valve
  updatecolumn(); // Update data column
  
  lasterr = error;
  error = setpoint - suhu; //Hitung error
  Serial.print(error);
  float ediff = lasterr - error;  
  edot = ediff / timediff; // Hitung error derivative
  Serial.print(edot);
  fuzzificationinput();
  defuzzification();
  stateon();
}
