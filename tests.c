#include "tuple_space.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
TupleSpace* tSpace = (TupleSpace*)malloc(sizeof(TupleSpace));
tSpace->count = 0;
pthread_mutex_init(&tSpace->mutex, NULL);

// Inicjalizacja TupleSpaceEntries
for (int i = 0; i < MAX_TUPLES; i++) {
    tSpace->entries[i].tuple = NULL;
    tSpace->entries[i].key = 0;
for (int i = 0; i < tSpace->count; i++) {
    freeTupleSpaceEntry(&tSpace->entries[i]);
}

// Zwolnienie pamięci TupleSpace
pthread_mutex_destroy(&tSpace->mutex);
free(tSpace);

int main() {
    uint8_t *buffer = (uint8_t *)malloc(ALP_MESSAGE_SIZE * sizeof(uint8_t));
    memset(buffer, 0, ALP_MESSAGE_SIZE * sizeof(uint8_t));

    // Creating a sample Tuple structure
    Tuple myTuple;
    myTuple.num_fields = 3;
    myTuple.fields = (Field *)malloc(myTuple.num_fields * sizeof(Field));
    memset(myTuple.fields, 0, myTuple.num_fields * sizeof(Field));
    myTuple.fields[0].is_actual = YES;
    myTuple.fields[0].type = TS_STRING;
    myTuple.fields[0].data.string_field.value = "Hello";
    myTuple.fields[0].data.string_field.length = strlen(myTuple.fields[0].data.string_field.value); // strlen("Hello")
    myTuple.fields[1].is_actual = YES;
    myTuple.fields[1].type = TS_INT;
    myTuple.fields[1].data.int_field = 123;
    myTuple.fields[2].is_actual = YES;
    myTuple.fields[2].type = TS_FLOAT;
    myTuple.fields[2].data.float_field = 3.14;

    ALPMessage *alp_message = (ALPMessage *)malloc(ALP_MESSAGE_SIZE * sizeof(uint8_t));
    alp_message->op_type = ALP_OUT;
    alp_message->tuple = &myTuple;
    alp_message->msg_len = serialize_tuple(&myTuple, buffer, ALP_OUT);
    printf("dlugosc: %d\n", alp_message->msg_len);
    //....
    ALPMessage *received_alp_message = (ALPMessage *)malloc(ALP_MESSAGE_SIZE * sizeof(uint8_t));
    deserialize_alp_message(buffer, received_alp_message);


    printf("Deserialized ALP message:\n");

    printTuple(received_alp_message->tuple);
    // printBufferInBinary(buffer, received_alp_message->msg_len);

    // TupleSpace myTupleSpace;
    // out(&myTupleSpace, &myTuple);
    
    // wyslanie UDP...

    // Tuple *received = (Tuple *)malloc(sizeof(Tuple));

    // odbior UDP
    // deserialize_tuple(buffer, received);
    // printTuple(received);
    // printBufferInBinary(buffer, msg_len);
    // printTuple(received);

    // odbior UDP
    // deserialize_tuple(buffer, received);
    // printTuple(received);
    // printBufferInBinary(buffer, msg_len);
    // tupleToString(received);

    // free_tuple(received);
    free(buffer);

    return 0;
}