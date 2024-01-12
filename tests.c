#include "tuple_space.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void printBufferInBinary(uint8_t *buffer, uint16_t length) {
    for (int i = 0; i < length; i++) {
        for (int j = 7; j >= 0; j--) {
            printf("%d", (buffer[i] >> j) & 1);
        }
        printf(" ");
    }
    printf("\n");
}

int main() {
    uint8_t *buffer = (uint8_t *)malloc(BUFSIZE * sizeof(uint8_t));
    memset(buffer, 0, BUFSIZE * sizeof(uint8_t));

    // Creating a sample Tuple structure
    Tuple myTuple;
    myTuple.num_fields = 3;
    myTuple.fields = (Field *)malloc(myTuple.num_fields * sizeof(Field));
    memset(myTuple.fields, 0, myTuple.num_fields * sizeof(Field));
    myTuple.fields[0].type = TS_STRING;
    myTuple.fields[0].data.string_field.value = "Hello";
    myTuple.fields[0].data.string_field.length = strlen(myTuple.fields[0].data.string_field.value); // strlen("Hello")
    myTuple.fields[1].type = TS_INT;
    myTuple.fields[1].data.int_field = 123;
    myTuple.fields[2].type = TS_FLOAT;
    myTuple.fields[2].data.float_field = 3.14;
    TupleSpace myTupleSpace;
    out(myTupleSpace, 1, &myTuple);

    uint16_t msg_len = serialize_tuple(&myTuple, buffer);
    printf("Message length: %d\n", msg_len);

    Tuple *received = (Tuple *)malloc(sizeof(Tuple));

    received->num_fields = myTuple.num_fields;
    received->fields = (Field *)malloc(received->num_fields * sizeof(Field));

    deserialize_tuple(buffer, received);
    printTuple(received);
    printBufferInBinary(buffer, msg_len);
    tupleToString(received);

    free_tuple(received);
    free(buffer);

    return 0;
}