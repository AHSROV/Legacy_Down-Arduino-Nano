#include <SPI.h>
#include <Ethernet.h>

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network.
// gateway and subnet are optional:
byte mac[] = { 
  0x90, 0xA2, 0xDA, 0x0F, 0x0A, 0x08};
IPAddress ip(192,168,1, 177);
long temperatureIntervalMillis = 1000;

int relayPin[] = {
  6, 7, A3, A4, 8};
int conductivitySensorRead = A6;
int conductivitySensorPower = 9; 
int videoPin[] = {
  A0, A1, A2};
int floodSensorPin = A7;

boolean relayState[5] = {
  false, false, false, false, false};
boolean videoPinState[3] = {
  false, false, false};
boolean conductivitySensorState = false;
int activeCamera = 0;

// telnet defaults to port 23
EthernetServer server(23);
boolean alreadyConnected = false; // whether or not the client was connected previously

void setup() {
  // initialize the ethernet device
  Ethernet.begin(mac, ip);
  // start listening for clients
  server.begin();
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  Serial.print("Server address: ");
  Serial.println(Ethernet.localIP());

  UpdateAll();
  SetPinModes();
}

char msgBuffer[10];
int i=0;
unsigned long lastTime;
void loop() {
  // Get milliseconds program has been running.
  unsigned long milliseconds = millis();

  // wait for a new client:
  EthernetClient client = server.available();
  if (client.available() > 0)
  {  
    if (i >= sizeof(msgBuffer))
    {
      Serial.println("Ethernet overflow");
      i = 0; 
    }

    msgBuffer[i] = (byte)client.read();
    i++;

    // wait for end of line
    if(msgBuffer[i-1] == 10 || msgBuffer[i-1] == 13)
    {
      msgBuffer[i-1] = 0;
      if(msgBuffer[0] == 'r')
      {
        ProcessRelayCommand(msgBuffer);
      }
      else if(msgBuffer[0] == 'v')
      {
        ProcessVideoCommand(msgBuffer);
      }
      else if(msgBuffer[0] == 'c')
      {
        ProcessConductivityCommand(msgBuffer);
      }
      else if(msgBuffer == "pingDown")
      {
        Serial.println("pongDown");
      }
      i = 0;
    }
  }

  // Send conductivty reading
  if(milliseconds - lastTime > temperatureIntervalMillis)
  {
    lastTime = milliseconds;
    if(conductivitySensorState == true)
    {
      int value = analogRead(conductivitySensorRead);
      server.print("c");
      server.println(value);
    }

    int floodValue = analogRead(floodSensorPin);
    if(floodValue < 1000)
    {
      server.println("!");
    }
    else
    {
      server.println(".");
    }
  }
}

int GetValFromString(char *p, int length) 
{
  int v=0;
  while(isDigit(*p) && length > 0)
  {
    v = v*10 + *p - '0';
    p++; 
    length--;
  }
  return v;
}

void ProcessConductivityCommand(char str[])
{
  int state = GetValFromString(&msgBuffer[1], 1);
  switch(state)
  {
  case 1:
    conductivitySensorState = true;
    break;
  case 0:
    conductivitySensorState = false;
    break;
  }
  Serial.print("Conductivity sensor: ");
  Serial.println(conductivitySensorState);  
  UpdateConductivitySensor();
}

void ProcessVideoCommand(char str[])
{
  if(msgBuffer[1] == 'c')
  {
    server.print("v");
    server.println(activeCamera);
    Serial.print("Sent active camera: ");
    Serial.println(activeCamera);
  }
  else
  {
    int camera = GetValFromString(&msgBuffer[1], 1);
    if(camera <= 7)
    {
      for(int i = 0; i < 3; i++)
      {
        boolean b = bitRead(camera, i);
        activeCamera = camera;
        videoPinState[i] = b;
        Serial.print("Pin: ");
        Serial.print(i);
        Serial.print(" at ");
        Serial.println(b);
      }
      UpdateVideoPins();
    }
  }
}

void ProcessRelayCommand(char str[])
{
  int relayNumber = GetValFromString(&msgBuffer[1], 1);
  if(relayNumber <= 4) 
  {
    char state = msgBuffer[2];
    switch(state)
    {
    case '0':
      relayState[relayNumber] = false;
      break;
    case '1':
      relayState[relayNumber] = true;
      break;
    case 'c':
      server.print("r");
      server.print(relayNumber);
      server.println(relayState[relayNumber]);
      break;
    }
    Serial.print("Relay: ");
    Serial.print(relayNumber);
    Serial.print(" at ");
    Serial.println(relayState[relayNumber]);
    UpdateRelays();
  }
  else
  {
    Serial.println("Relay number too large");
  }
}

void UpdateConductivitySensor() 
{
  // The conductivity sensor is held "off" by holding the gate on the MOSFET up
  digitalWrite(conductivitySensorPower, conductivitySensorState ? LOW : HIGH);
}

void UpdateRelays() 
{
  for(int i = 0; i < 5; i++)
  {
    digitalWrite(relayPin[i], relayState[i]);
  }
}

void UpdateVideoPins()
{
  for(int i = 0; i < 3; i++)
  {
    digitalWrite(videoPin[i], videoPinState[i]);
  }
}

void UpdateAll()
{
  UpdateVideoPins();
  UpdateRelays();
  UpdateConductivitySensor();
}

void SetPinModes()
{
  for(int i = 0; i < 5; i++)
  {
    pinMode(relayPin[i], OUTPUT);
  }

  for(int i = 0; i < 3; i++)
  {
    pinMode(videoPin[i], OUTPUT);
  }

  pinMode(floodSensorPin, INPUT);
  pinMode(conductivitySensorRead, INPUT);
  pinMode(conductivitySensorPower, OUTPUT);
}



