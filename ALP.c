//#ifndef ALP_H
//#define ALP_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ALP_OUT 0x01  // 0000 0001
#define ALP_IN  0x02  // 0000 0010
#define ALP_INP 0x03  // 0000 0100
#define ALP_RD  0x04  // 0000 1000
#define ALP_RDP 0x05  // 0001 0000
#define ALP_ACK 0x06  // 0010 0000

#define TS_INT 0
#define TS_FLOAT 1
#define INT_SIZE sizeof(int)
#define FLOAT_SIZE sizeof(float)
#define YES 1
#define NO 0
#define TS_SUCCESS 1
#define TS_FAIL 0

#define MAX_RETRIES 3

// Tuple field structure: | TYPE | DATA |
typedef struct {
    uint8_t is_actual; // YES or NO
    uint8_t type;      // TS_INT or TS_FLOAT
    union {
        int int_field;
        float float_field;
    } data;
} Field;

// Structure for Tuple
typedef struct {
    uint8_t num_fields;
    Field *fields; // Change to pointer to Field
} Tuple;

// ALP message structure
typedef struct {
    uint8_t op_type;
    uint16_t msg_len;
    Tuple *tuple;
} ALPMessage;

char ** convertTupleToBits(Tuple *tuple) {
    // Allocate memory for an array of binary representations
    char **binaryRepresentations = (char **)malloc(tuple->num_fields * sizeof(char *));

    for (int i = 0; i < tuple->num_fields; ++i) {
        int maxBinarySize = (tuple->fields[i].type == TS_INT) ? sizeof(int) * 8 : sizeof(float) * 8;
        binaryRepresentations[i] = (char *)malloc(maxBinarySize + 1);  // +1 for null terminator

        if (tuple->fields[i].type == TS_INT) {
            // Example conversion for int (32 bits)
            uint32_t int_bits;
            memcpy(&int_bits, &tuple->fields[i].data.int_field, sizeof(int));
            for (int j = 0; j < sizeof(int) * 8; j++) {
                binaryRepresentations[i][j] = '0' + ((int_bits >> (sizeof(int) * 8 - 1 - j)) & 1);

        }binaryRepresentations[i][sizeof(int) * 8] = '\0';  // Null-terminate the string
        } else if (tuple->fields[i].type == TS_FLOAT) {
            // Example conversion for float (32 bits)
            uint32_t float_bits;
            memcpy(&float_bits, &tuple->fields[i].data.float_field, sizeof(float));
            for (int j = 0; j < sizeof(float) * 8; j++) {
                binaryRepresentations[i][j] = '0' + ((float_bits >> (sizeof(float) * 8 - 1 - j)) & 1);
            }
            binaryRepresentations[i][sizeof(float) * 8] = '\0';  // Null-terminate the string
        }
        // Add handling for other types if necessary
    }
    return binaryRepresentations;
}


int convertBinaryToInt(const char *binary) {
    int result = 0;
    int length = strlen(binary);

    for (int i = 0; i < length; ++i) {
        result = (result << 1) | (binary[i] - '0');
    }

    return result;
}

float convertBinaryToFloat(const char *binary) {
    union {
        uint32_t int_bits;
        float float_value;
    } data;

    int length = strlen(binary);

    for (int i = 0; i < length; ++i) {
        data.int_bits = (data.int_bits << 1) | (binary[i] - '0');
    }

    return data.float_value;
}

Tuple *convertBitsToTuple(char **binaryRepresentations, int numFields) {
    Tuple *tuple = (Tuple *)malloc(sizeof(Tuple));
    tuple->num_fields = numFields;
    tuple->fields = (Field *)malloc(numFields * sizeof(Field));

    for (int i = 0; i < numFields; ++i) {
        if (strchr(binaryRepresentations[i], 'b') == NULL) {
            tuple->fields[i].type = TS_INT;
            tuple->fields[i].data.int_field = convertBinaryToInt(binaryRepresentations[i]);
        } else {
            tuple->fields[i].type = TS_FLOAT;
            tuple->fields[i].data.float_field = convertBinaryToFloat(binaryRepresentations[i]);
        }
    }

    return tuple;
}

void printTuple(Tuple *tuple) {
    for (int i = 0; i < tuple->num_fields; ++i) {
        if (tuple->fields[i].type == TS_INT) {
            printf("Field %d (Type: INT): %d\n", i + 1, tuple->fields[i].data.int_field);
        } else if (tuple->fields[i].type == TS_FLOAT) {
            printf("Field %d (Type: FLOAT): %f\n", i + 1, tuple->fields[i].data.float_field);
        }
        // Add handling for other types if necessary
    }
}



int main() {
    ALPMessage myMessage;
    myMessage.op_type = 0x05;   // Sample value for op_type
    myMessage.msg_len = 100;    // Sample value for msg_len

    // Creating a sample Tuple structure
    Tuple myTuple;
    myTuple.num_fields = 2;
    myTuple.fields = (Field *)malloc(myTuple.num_fields * sizeof(Field));
    myTuple.fields[0].type = TS_INT;
    myTuple.fields[0].data.int_field = 123;
    myTuple.fields[1].type = TS_FLOAT;
    myTuple.fields[1].data.float_field = 3.14;

    // Conversion of Tuple to bits
    char **binaryRepresentations = convertTupleToBits(&myTuple);
    for (int n = 0; n < myTuple.num_fields; ++n) {
        printf("Binary Representation %d: %s\n", n + 1, binaryRepresentations[n]);
        free(binaryRepresentations[n]);  // Free memory for each binary representation
}

    Tuple *newTuple = convertBitsToTuple(binaryRepresentations, myTuple.num_fields);
    printTuple(newTuple);



    // Example of sending/receiving ALPMessage structure as bits
    // ...

    // Free memory allocated for Tuple fields
    free(newTuple->fields);
    free(myTuple.fields);
    free(binaryRepresentations);

    return 0;
}

//#endif
