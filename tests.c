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

    // Creating a sample Tuple structure
    Tuple myTuple;
    myTuple.num_fields = 3;
    myTuple.fields = (Field *)malloc(myTuple.num_fields * sizeof(Field));
    myTuple.fields[0].type = TS_STRING;
    myTuple.fields[0].data.string_field = "Hello";
    myTuple.fields[1].type = TS_INT;
    myTuple.fields[1].data.int_field = 123;
    myTuple.fields[2].type = TS_FLOAT;
    myTuple.fields[2].data.float_field = 3.14;

    uint16_t msg_len = serialize_tuple(&myTuple, buffer);
    printf("Message length: %d\n", msg_len);

    Tuple received = deserialize_tuple(buffer);
    tupleToString(&received);
    printBufferInBinary(buffer, msg_len);

    printf("Field 1: %s\n", received.fields[0].data.string_field);
    printf("Field 2: %d\n", received.fields[1].data.int_field);
    printf("Field 3: %f\n", received.fields[2].data.float_field);

    free(myTuple.fields);
    free(buffer);

    return 0;
}