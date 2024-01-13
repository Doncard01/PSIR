#include <ZsutDhcp.h>          
#include <ZsutEthernet.h>       
#include <ZsutEthernetUdp.h>    
#include <ZsutFeatures.h>
#include "tuple_space.h"

#define BUFSIZE 256
#define UDP_PORT 9545
#define PERIOD 1000

unsigned long time_now = 0;
ZsutIPAddress TupleSpace_IP = ZsutIPAddress(169,254,97,104);
byte mac[]={0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01}; 

uint8_t packetBuffer[BUFSIZE];
uint8_t sendBuffer[BUFSIZE];
uint8_t tuple_buffer[BUFSIZE];

unsigned int localPort=UDP_PORT;    

uint16_t msg_len;

ZsutEthernetUDP Udp;

void setup() {
    memset(&packetBuffer, 0, sizeof(packetBuffer));
    memset(&sendBuffer, 0, sizeof(sendBuffer));
    memset(&tuple_buffer, 0, BUFSIZE * sizeof(uint8_t));

    //krotka testowa
    Tuple myTuple;
    myTuple.num_fields = 3;
    myTuple.fields = (Field *)malloc(myTuple.num_fields * sizeof(Field));
    memset(myTuple.fields, 0, myTuple.num_fields * sizeof(Field));
    myTuple.fields[0].type = TS_STRING;
    myTuple.fields[0].data.string_field.value = "Hello";
    myTuple.fields[0].data.string_field.length = strlen(myTuple.fields[0].data.string_field.value);
    myTuple.fields[1].type = TS_INT;
    myTuple.fields[1].data.int_field = 123;
    myTuple.fields[2].type = TS_FLOAT;
    myTuple.fields[2].data.float_field = 3.14;

    msg_len = serialize_tuple(&myTuple, tuple_buffer);

    Serial.begin(115200);
    Serial.print(F("Manager init... ["));Serial.print(F(__FILE__));
    Serial.print(F(", "));Serial.print(F(__DATE__));Serial.print(F(", "));Serial.print(F(__TIME__));Serial.println(F("]"));   
    ZsutEthernet.begin(mac);

    Serial.print(F("My IP address: "));
    for (byte thisByte = 0; thisByte < 4; thisByte++) {
        Serial.print(ZsutEthernet.localIP()[thisByte], DEC);Serial.print(F("."));
    }
    Serial.println();

    Udp.begin(localPort);

    char* hello_msg = "HELLO, it's Manager.";
    int len=strlen(hello_msg);
    for (int i=0; i<len; i++) {
        sendBuffer[i] = hello_msg[i]; 
    }

    Udp.beginPacket(TupleSpace_IP, UDP_PORT);
    Udp.write(sendBuffer, len);
    Udp.endPacket();
    memset(&sendBuffer, 0, sizeof(sendBuffer));

    }

void loop() {
    time_now = ZsutMillis();
   
    Serial.println("Another cycle.");
   
    while(millis() < time_now + PERIOD){
        //wait approx. [PERIOD] ms
    }
    //odbior pakietu
    int packetSize=Udp.parsePacket(); 
    if(packetSize>0) {
        //czytamy pakiet - maksymalnie do 'BUFSIZE' bajtow
        int len=Udp.read(packetBuffer, BUFSIZE);

        Serial.print("Recieved: ");
        packetBuffer[len]='\0';
        Serial.println((char*)packetBuffer);
        memset(&packetBuffer, 0, sizeof(packetBuffer));
  
    } else {
      Serial.println("Sending tuple...");
      Udp.beginPacket(TupleSpace_IP, UDP_PORT);
      Udp.write(tuple_buffer, msg_len);
      Udp.endPacket();
      Serial.println("Tuple sent.");
    }
    }
