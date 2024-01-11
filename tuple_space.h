#ifndef ALP_H
#define ALP_H

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

#define TS_INT 0x01
#define TS_FLOAT 0x02
#define TS_STRING 0x04
#define INT_FIELD_SIZE sizeof(int)
#define FLOAT_FIELD_SIZE sizeof(float)
#define YES 1
#define NO 0
#define TS_SUCCESS 1
#define TS_FAIL 0

#define MAX_RETRIES 3
#define TYPE_FIELD_SIZE 1 //byte
#define NUM_FIELD_SIZE 1 //byte
// #define INT_FIELD_SIZE 2 //bytes, 16 bits
// #define FLOAT_FIELD_SIZE 4 //bytes, 32 bits

#define BUFSIZE 1469 //max tuple length in bytes

// Tuple field structure: | TYPE | DATA |
typedef struct {
    uint8_t is_actual; // YES or NO
    uint8_t type;      // TS_INT, TS_FLOAT or TS_STRING
    union {
        int int_field;
        float float_field;
        char* string_field;
    } data;
} Field;

// Structure for Tuple | Nr. of fields (1B) | Field 1 | Field 2 | ... | Field n |
typedef struct {
    uint8_t num_fields;
    Field *fields;
} Tuple;
#include <stdio.h>

void tupleToString(Tuple *tuple) {
    printf("Printing Tuple with %d fields:\n", tuple->num_fields);
    printf("(");
    for (int i = 0; i < tuple->num_fields; ++i) {
        switch (tuple->fields[i].type) {
            case TS_INT:
                printf("%d", tuple->fields[i].data.int_field);
                break;
            case TS_FLOAT:
                printf("%f", tuple->fields[i].data.float_field);
                break;
            case TS_STRING:
                printf("%s", tuple->fields[i].data.string_field);
                break;
            default:
                printf("Unknown type");
                break;
        }
        if (i != tuple->num_fields - 1) {
            printf(", ");
        }
    }
    printf(")\n");
}

// ALP message structure
typedef struct {
    uint8_t op_type;
    uint16_t msg_len;
    Tuple *tuple;
} ALPMessage;

// char ** convertTupleToBits(Tuple *tuple) {
//     // Allocate memory for an array of binary representations
//     char **binaryRepresentations = (char **)malloc(tuple->num_fields * sizeof(char *));

//     for (int i = 0; i < tuple->num_fields; ++i) {
//         int maxBinarySize = (tuple->fields[i].type == TS_INT) ? sizeof(int) * 8 : sizeof(float) * 8;
//         binaryRepresentations[i] = (char *)malloc(maxBinarySize + 1);  // +1 for null terminator

//         if (tuple->fields[i].type == TS_INT) {
//             // Example conversion for int (32 bits)
//             uint32_t int_bits;
//             memcpy(&int_bits, &tuple->fields[i].data.int_field, sizeof(int));
//             for (int j = 0; j < INT_FIELD_SIZE * 8; j++) {
//                 binaryRepresentations[i][j] = '0' + ((int_bits >> (sizeof(int) * 8 - 1 - j)) & 1);

//         }binaryRepresentations[i][sizeof(int) * 8] = '\0';  // Null-terminate the string
//         } else if (tuple->fields[i].type == TS_FLOAT) {
//             // Example conversion for float (32 bits)
//             uint32_t float_bits;
//             memcpy(&float_bits, &tuple->fields[i].data.float_field, sizeof(float));
//             for (int j = 0; j < FLOAT_FIELD_SIZE * 8; j++) {
//                 binaryRepresentations[i][j] = '0' + ((float_bits >> (sizeof(float) * 8 - 1 - j)) & 1);
//             }
//             binaryRepresentations[i][sizeof(float) * 8] = '\0';  // Null-terminate the string
//         }
//         // Add handling for other types if necessary
//     }
//     return binaryRepresentations;
// }


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

uint16_t serialize_tuple(Tuple *tuple, uint8_t *buffer) {
    uint8_t *ptr = buffer;

    uint16_t length = 1; //tuple length in bytes, always starts with num_fields
    memcpy(ptr, &tuple->num_fields, NUM_FIELD_SIZE);
    ptr += NUM_FIELD_SIZE;

    for (int i = 0; i < tuple->num_fields; i++) {
        memcpy(ptr, &tuple->fields[i].type, TYPE_FIELD_SIZE);
        ptr += TYPE_FIELD_SIZE;
        length += TYPE_FIELD_SIZE;
        if (tuple->fields[i].type == TS_INT) {
            length += INT_FIELD_SIZE;
            ptr += INT_FIELD_SIZE;
            memcpy(ptr, &tuple->fields[i].data.int_field, sizeof(tuple->fields[i].data.int_field));
        } else if (tuple->fields[i].type == TS_FLOAT) {
            length += FLOAT_FIELD_SIZE;
            ptr += FLOAT_FIELD_SIZE;
            memcpy(ptr, &tuple->fields[i].data.float_field, sizeof(tuple->fields[i].data.float_field));
        } else if (tuple->fields[i].type == TS_STRING) {
            uint8_t field_length = strlen(tuple->fields[i].data.string_field);
            memcpy(ptr, &tuple->fields[i].data.string_field, field_length);
            ptr += field_length;
            length += field_length;
            *(ptr++) = '\0';
            length += 1;
        }
    }

    return length;
}

Tuple deserialize_tuple(uint8_t *buffer) {
    Tuple tuple;

    uint8_t *ptr = buffer;

    tuple.num_fields = *(uint8_t *)ptr;
    ptr += NUM_FIELD_SIZE;

    Field fields[tuple.num_fields];
    tuple.fields = fields;

    for (int i = 0; i < tuple.num_fields; i++) {
        tuple.fields[i].type = *(uint8_t *)ptr;
        ptr += TYPE_FIELD_SIZE;
        if (tuple.fields[i].type == TS_INT) {
            tuple.fields[i].data.int_field = *(int *)ptr;
            ptr += INT_FIELD_SIZE;
        } else if (tuple.fields[i].type == TS_FLOAT) {
            tuple.fields[i].data.float_field = *(float *)ptr;
            ptr += FLOAT_FIELD_SIZE;
        } else if (tuple.fields[i].type == TS_STRING) {
            uint8_t string_length = 1;
            char next_char = *(char *)ptr;
            while (next_char != '\0') {
                ptr++;
                string_length++;
                next_char = *(char *)ptr;
            }
            char* string = (char *)malloc(string_length * sizeof(char));
            for (int j = 0; j <= string_length; j++) {
                if (j == string_length) {
                    string[j] = '\0';
                    break;
                }
                string[j] = *(char *)(ptr - string_length + j);
            }
            tuple.fields[i].data.string_field = string;
            ptr++;
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


#endif
