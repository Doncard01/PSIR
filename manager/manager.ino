#include <ZsutDhcp.h>          
#include <ZsutEthernet.h>       
#include <ZsutEthernetUdp.h>    
#include <ZsutFeatures.h>
#include "tuple_space.h"

#define UDP_PORT 9545
#define PERIOD 1000

unsigned long time_now = 0;
ZsutIPAddress TupleSpace_IP = ZsutIPAddress(169,254,97,104);
byte mac[]={0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x01}; 

uint8_t packetBuffer[ALP_MESSAGE_SIZE];
uint8_t sendBuffer[ALP_MESSAGE_SIZE];
uint8_t alpBuffer[ALP_MESSAGE_SIZE];

unsigned int localPort=UDP_PORT;    

uint16_t msg_len;

ZsutEthernetUDP Udp;

// int alp_send(ZsutEthernetUDP &udp, ZsutIPAddress dest_ip, int dest_port, Tuple *tuple, uint8_t type) {
//     ALPMessage alpMsg;
//     alpMsg.op_type = type;
//     alpMsg.tuple = tuple;

//     uint8_t alpBuffer[TUPLE_SIZE];
//     memset(alpBuffer, 0,  TUPLE_SIZE);
//     if (myALPMessage.tuple != NULL) {
//     // Bezpieczne użycie myALPMessage.tuple
//     } else {
//         // Obsługa przypadku, gdy myALPMessage.tuple jest NULL
//     }
//     alpMsg.msg_len = serialize_tuple(alpMsg.tuple, alpBuffer + TYPE_FIELD_SIZE);

//     udp.beginPacket(dest_ip, dest_port);
//     udp.write(alpBuffer, alpMsgLen + TYPE_FIELD_SIZE);
//     udp.endPacket();

//     return TS_SUCCESS;
// }

void setup() {
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

    memset(&packetBuffer, 0, sizeof(packetBuffer));
    memset(&sendBuffer, 0, sizeof(sendBuffer));

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

    //HELLO
    // alp_hello.op_type = ALP_HELLO;
    // alp_hello.msg_len = serialize_alp_message(NULL, sendBuffer);

    // Udp.beginPacket(TupleSpace_IP, UDP_PORT);
    // Udp.write(sendBuffer, alp_hello.msg_len);
    // Udp.endPacket();
    // memset(&sendBuffer, 0, sizeof(sendBuffer));

    //TS_OUT
    
    // alp_out.op_type = ALP_OUT;
    // alp_out.tuple = &myTuple;
    // alp_hello.msg_len = serialize_alp_message(&alp_out, sendBuffer);

    // Udp.beginPacket(TupleSpace_IP, UDP_PORT);
    // Udp.write(sendBuffer, alp_hello.msg_len);
    // Udp.endPacket();
    // memset(&sendBuffer, 0, sizeof(sendBuffer));
    }

void loop() {
    time_now = ZsutMillis();

    Serial.println("Another cycle.");

    while(millis() < time_now + PERIOD){
        //wait approx. [PERIOD] ms
    }

    memset(&sendBuffer, 0, sizeof(sendBuffer));
    memset(&packetBuffer, 0, sizeof(packetBuffer));
    memset(&alpBuffer, 0, sizeof(alpBuffer));

    Tuple myTuple1;
    myTuple1.num_fields = 3;
    myTuple1.fields = (Field *)malloc(myTuple1.num_fields * sizeof(Field));
    memset(myTuple1.fields, 0, myTuple1.num_fields * sizeof(Field));
    myTuple1.fields[0].is_actual = YES;
    myTuple1.fields[0].type = TS_STRING;
    myTuple1.fields[0].data.string_field.value = "otherTuple";
    myTuple1.fields[0].data.string_field.length = strlen(myTuple1.fields[0].data.string_field.value);
    myTuple1.fields[1].is_actual = YES;
    myTuple1.fields[1].type = TS_INT;
    myTuple1.fields[1].data.int_field = 123;
    myTuple1.fields[2].is_actual = YES;
    myTuple1.fields[2].type = TS_FLOAT;
    myTuple1.fields[2].data.float_field = 3.14;

    ALPMessage alp_out;
    alp_out.op_type = ALP_OUT;
    alp_out.tuple = &myTuple1;
    alp_out.msg_len = serialize_tuple(alp_out.tuple, sendBuffer, alp_out.op_type);

    //odbior pakietu
    int packetSize=Udp.parsePacket(); 
    if(packetSize>0) {
        //czytamy pakiet - maksymalnie do 'TUPLE_SIZE' bajtow
        int len=Udp.read(packetBuffer, TUPLE_SIZE);

        Serial.print("Recieved: ");
        packetBuffer[len]='\0';
        Serial.println((char*)packetBuffer);
    } else {
        Serial.println("Sending tuple...");
        Udp.beginPacket(TupleSpace_IP, UDP_PORT);
        Udp.write(sendBuffer, alp_out.msg_len);
        Udp.endPacket();
        Serial.println("Tuple sent.");
        }   
    }
