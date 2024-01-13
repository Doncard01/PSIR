#ifndef TUPLE_SPACE_H
#define TUPLE_SPACE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ALP_OUT 0x01  // 0000 0001
#define ALP_IN  0x02  // 0000 0010
#define ALP_INP 0x04  // 0000 0100
#define ALP_RD  0x08  // 0000 1000
#define ALP_RDP 0x10  // 0001 0000
#define ALP_ACK 0x20  // 0010 0000
#define ALP_HLL 0x40  // 0100 0000

#define TS_INT 0x01
#define TS_FLOAT 0x02
#define TS_STRING 0x04
#define INT_FIELD_SIZE 2
#define FLOAT_FIELD_SIZE 4
#define YES 1
#define NO 0
#define TS_SUCCESS 1
#define TS_FAIL 0
#define MAX_TUPLES 50

#define MAX_RETRIES 3
#define TYPE_FIELD_SIZE 1 //byte
#define NUM_FIELD_SIZE 1 //byte
// #define INT_FIELD_SIZE 2 //bytes, 16 bits
// #define FLOAT_FIELD_SIZE 4 //bytes, 32 bits

#define BUFSIZE 256 //max tuple length in bytes

typedef struct {
    uint8_t length;
    char* value;
} StringField;

// Tuple field structure: | TYPE | DATA |
typedef struct {
    uint8_t is_actual; // YES or NO
    uint8_t type;      // TS_INT, TS_FLOAT or TS_STRING
    union {
        int int_field;
        float float_field;
        StringField string_field;
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
                printf("%s", tuple->fields[i].data.string_field.value);
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


uint16_t serialize_tuple(Tuple *tuple, uint8_t *buffer) {
    uint8_t *ptr = buffer;

    uint16_t length = 1; //tuple length in bytes, always starts with num_fields
    memcpy(ptr, &tuple->num_fields, NUM_FIELD_SIZE);
    ptr += NUM_FIELD_SIZE;

    for (int i = 0; i < tuple->num_fields; i++) {
        *(ptr++) = tuple->fields[i].type;
        length += 1;
        length += TYPE_FIELD_SIZE;
        if (tuple->fields[i].type == TS_INT) {
            length += INT_FIELD_SIZE;
            memcpy(ptr, &tuple->fields[i].data.int_field, sizeof(tuple->fields[i].data.int_field));
            ptr += INT_FIELD_SIZE;
        } else if (tuple->fields[i].type == TS_FLOAT) {
            length += FLOAT_FIELD_SIZE;
            memcpy(ptr, &tuple->fields[i].data.float_field, sizeof(tuple->fields[i].data.float_field));
            ptr += FLOAT_FIELD_SIZE;
        } else if (tuple->fields[i].type == TS_STRING) {
            uint8_t field_length = tuple->fields[i].data.string_field.length;
            *(ptr++) = field_length;
            length += 1;
            strcpy((char *)ptr, tuple->fields[i].data.string_field.value);
            ptr += field_length;
            length += field_length;
        }
    }

    return length;
}

void deserialize_tuple(uint8_t *buffer, Tuple *tuple) {
    uint8_t *ptr = buffer;

    tuple->num_fields = *(uint8_t *)ptr;
    ptr += NUM_FIELD_SIZE;

    tuple->fields = (Field *)malloc(tuple->num_fields * sizeof(Field));

    for (int i = 0; i < tuple->num_fields; i++) {
        tuple->fields[i].type = *(uint8_t *)ptr;
        ptr += 1;
        if (tuple->fields[i].type == TS_INT) {
            tuple->fields[i].data.int_field = *(uint16_t *)ptr;
            ptr += INT_FIELD_SIZE;
        } else if (tuple->fields[i].type == TS_FLOAT) {
            tuple->fields[i].data.float_field = *(float *)ptr;
            ptr += FLOAT_FIELD_SIZE;
        } else if (tuple->fields[i].type == TS_STRING) {
            uint8_t field_length = *(uint8_t *)ptr; // Read the length of the string field
            tuple->fields[i].data.string_field.length = field_length;
            ptr += 1;
            tuple->fields[i].data.string_field.value = (char *)malloc(field_length + 1);
            strncpy(tuple->fields[i].data.string_field.value, (char *)ptr, field_length);
            tuple->fields[i].data.string_field.value[field_length] = '\0';
            ptr += field_length;
        }
    }
}

void free_tuple(Tuple *tuple) {
    for (int i = 0; i < tuple->num_fields; ++i) {
        if (tuple->fields[i].type == TS_STRING) {
            free(tuple->fields[i].data.string_field.value);
        }
    }
    free(tuple->fields);
    free(tuple);
}


void printTuple(Tuple *tuple) {
    for (int i = 0; i < tuple->num_fields; ++i) {
        if (tuple->fields[i].type == TS_INT) {
            printf("Field %d (Type: INT): %d\n", i, tuple->fields[i].data.int_field);
        } else if (tuple->fields[i].type == TS_FLOAT) {
            printf("Field %d (Type: FLOAT): %f\n", i, tuple->fields[i].data.float_field);
        } else if (tuple->fields[i].type == TS_STRING) {
            printf("Field %d (Type: STRING): %s[len=%d]\n", i, tuple->fields[i].data.string_field.value, 
            tuple->fields[i].data.string_field.length);    
        }
    }
}

void printBufferInBinary(uint8_t *buffer, uint16_t length) {
    for (int i = 0; i < length; i++) {
        for (int j = 7; j >= 0; j--) {
            printf("%d", (buffer[i] >> j) & 1);
        }
        printf(" ");
    }
    printf("\n");
}

#endif
