// This #include statement was automatically added by the Particle IDE.
#include <Adafruit_SSD1306.h>

// This #include statement was automatically added by the Particle IDE.
#include <MQTT.h>

#define OLED_RESET 4
Adafruit_SSD1306 oled(OLED_RESET);

// door 1 - straight in front of house door
int door1Last = LOW; // closed,  HIGH is open
int door1Pin = D3;
static const char* door1Topic = "home1/garage/door/1";

// door 2 - off to right side
int door2Last = LOW; // closed, HIGH is open
int door2Pin = D4;
static const char* door2Topic = "home1/garage/door/2";

static const char* deviceTopic = "home1/garage/device/garagemonitor";

int lastPublishTime = 0;

void callback(char* topic, byte* payload, unsigned int length);
MQTT client("pi3_2", 1883, callback);

// function prototypes
void connect();
void displayTime();
void displayLine2Msg(const char* msg);
void publishDoorState(const char* topic, const char* state);
void publishDoor1();
void publishDoor2();


// recieve message
void callback(char* topic, byte* payload, unsigned int length)
{
    //char p[length + 1];
    //memcpy(p, payload, length);
    //p[length] = NULL;

    // fixme : re-publish door states
    publishDoor1();
    publishDoor2();
}

bool isDST(int day, int month, int dow)
{
    // inputs are assumed to be 1 based for month & day.  DOW is ZERO based!

    // jan, feb, and dec are out
    if(month < 3 || month > 11)
    {
        return false;
    }

    // april - oct is DST
    if(month > 3 && month < 11)
    {
        return true;
    }

    // in March, we are DST if our previous Sunday was on or after the 8th
    int previousSunday = day - dow;
    if(month == 3)
    {
        return previousSunday >= 8;
    }

    // for November, must be before Sunday to be DST ::  so previous Sunday must be before the 1st
    return previousSunday <= 0;
}

void setup()
{
    // start display
    oled.begin();
    oled.clearDisplay();

    pinMode(door1Pin, INPUT_PULLUP);
    pinMode(door2Pin, INPUT_PULLUP);

    int offset = -5;
    if(isDST(Time.day(), Time.month(), Time.weekday()-1))
    {
        offset = -4;
    }

     Time.zone(offset);

    connect();
}

void loop()
{
    if (client.isConnected())
    {
        client.loop();
    }
    else
    {
        connect();
    }

    displayTime();

    int door1 = digitalRead(door1Pin);
    int door2 = digitalRead(door2Pin);

    if((door1 == LOW) && (door2 == LOW))
    {
        displayLine2Msg("Both doors closed");
    }
    else if((door1 == HIGH) && (door2 == HIGH))
    {
        displayLine2Msg("Both doors open");
    }
    else
    {
        displayLine2Msg("One door open");
    }

    oled.display();

    if(door1 != door1Last)
    {
        door1Last = door1;
        publishDoor1();
    }

    if(door2 != door2Last)
    {
        door2Last = door2;
        publishDoor2();
    }

}

void connect()
{
    if(Particle.connected() == false)
    {
        Particle.connect();               //Connect to the Particle Cloud
        waitUntil(Particle.connected);    //Wait until connected to the Particle Cloud.
    }

    Particle.syncTime();

    // connect to the server

    if(!client.isConnected())
    {
        Particle.publish("MQTT", "Connecting", 60 , PRIVATE);
        client.connect("garagemonitor");
        Particle.publish("MQTT", "Connected", 60, PRIVATE);

        client.subscribe(deviceTopic);
    }
}

void displayTime()
{
    oled.clearDisplay();
    delay(200);

    oled.setTextSize(2);
    oled.setTextColor(WHITE);
    oled.setCursor(0,0);

    int hours = Time.hourFormat12();
    int mins  = Time.minute();
    int secs  = Time.second();
    if(hours < 10)
    {
        oled.print(" ");
    }
    oled.print(hours);
    oled.print(":");
    if(mins < 10)
    {
        oled.print("0");
    }
    oled.print(mins);
    oled.print(":");
    if(secs < 10)
    {
        oled.print("0");
    }
    oled.print(secs);
    if(Time.isAM())
    {
        oled.print("AM");
    }
    else
    {
        oled.print("PM");
    }
    oled.setTextColor(BLACK, WHITE); // 'inverted' text
}

void displayLine2Msg(const char* msg)
{
    oled.setTextSize(1);
    oled.setTextColor(WHITE);
    oled.setCursor(20, 36);
    oled.print(msg);
    oled.setTextColor(BLACK, WHITE); // inverted text
}

void publishDoorState(const char* topic, const char* state)
{
    Particle.publish("MQTT", "Publish", 60, PRIVATE);
    Particle.publish("LastPublishTime", Time.timeStr(), 60, PRIVATE);
    lastPublishTime = Time.now();
    if(!client.isConnected())
    {
        connect();
    }
    client.publish(topic, state);
}

void publishDoor1()
{
    const char* state = door1Last == LOW ? "CLOSED" : "OPEN";
    Particle.publish("GarageDoor1", state, 60, PRIVATE);
    publishDoorState(door1Topic, state);
}

void publishDoor2()
{
    const char* state = door2Last == LOW ? "CLOSED" : "OPEN";
    Particle.publish("GarageDoor2", state, 60, PRIVATE);
    publishDoorState(door2Topic, state);
}
