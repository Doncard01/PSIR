#ifndef TUPLE_SPACE_H
#define TUPLE_SPACE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ALP_OUT 0x01u  // 0000 0001
#define ALP_IN  0x02u  // 0000 0010
#define ALP_INP 0x04u  // 0000 0100
#define ALP_RD  0x08u  // 0000 1000
#define ALP_RDP 0x10u  // 0001 0000
#define ALP_ACK 0x20u  // 0010 0000
#define ALP_HELLO 0x40u  // 0100 0000
#define ALP_ERROR 0x80u  // 1000 0000
#define ALP_SUCCESS 0x30u  // 0011 0000

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
#define IS_ACTUAL_FIELD_SIZE 1 //byte
#define OP_TYPE_FIELD_SIZE 1 //byte
#define STRING_FIELD_LENGTH_SIZE 1 //byte
#define MSG_LENGTH_FIELD_SIZE 1 //bytes
#define NUM_FIELD_SIZE 1 //byte

#define TUPLE_SIZE 253 //max tuple length in bytes
#define ALP_MESSAGE_SIZE 255 //max ALP message length in bytes

typedef struct {
    uint8_t length;
    char* value;
} StringField;

// Tuple field structure: | TYPE | DATA |
typedef struct {
    uint8_t is_actual; // YES or NO
    uint8_t type;      // TS_INT, TS_FLOAT or TS_STRING
    union {
        int16_t int_field;
        float float_field;
        StringField string_field;
    } data;
} Field;

// Structure for Tuple | Nr. of fields (1B) | Field 1 | Field 2 | ... | Field n |
typedef struct {
    uint8_t num_fields;
    Field *fields;
} Tuple;



void printTuple(Tuple *tuple) {
    if (tuple == NULL) {
        printf("Invalid tuple pointer\n");
        return;
    }
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

char* tupleToString(Tuple *tuple) {
    if (tuple == NULL) {
        return NULL;
    }

    int bufferSize = 0;
    for (int i = 0; i < tuple->num_fields; i++) {
        switch (tuple->fields[i].type) {
            case TS_INT:
                bufferSize += INT_FIELD_SIZE + 3;
                break;
            case TS_FLOAT:
                bufferSize += FLOAT_FIELD_SIZE + 3;
                break;
            case TS_STRING:
                bufferSize += tuple->fields[i].data.string_field.length + 3;
                break;
            default:
                break;
        }
    }

    char* buffer = (char*)malloc(bufferSize + 1);
    if (buffer == NULL) {
        return NULL;
    }

    int offset = 0;
    for (int i = 0; i < tuple->num_fields; i++) {
        if (i != 0) {
            buffer[offset++] = ',';
            buffer[offset++] = ' ';
        }

        switch (tuple->fields[i].type) {
            case TS_INT:
                sprintf(buffer + offset, "(%d)", tuple->fields[i].data.int_field);
                offset += INT_FIELD_SIZE + 2;
                break;
            case TS_FLOAT:
                sprintf(buffer + offset, "(%f)", tuple->fields[i].data.float_field);
                offset += FLOAT_FIELD_SIZE + 2;
                break;
            case TS_STRING:
                buffer[offset++] = '(';
                strncpy(buffer + offset, tuple->fields[i].data.string_field.value, tuple->fields[i].data.string_field.length);
                offset += tuple->fields[i].data.string_field.length;
                buffer[offset++] = ')';
                break;
            default:
                break;
        }
    }

    buffer[bufferSize] = '\0';
    return buffer;
}


// ALP message structure
typedef struct {
    uint8_t op_type;
    uint16_t msg_len;
    Tuple *tuple;
} ALPMessage;


uint16_t serialize_tuple(Tuple *tuple, uint8_t *buffer, uint8_t op_type) {
    uint8_t *ptr = buffer;

    uint16_t length = 1; //tuple length in bytes, always starts with num_fields
    memcpy(ptr, &tuple->num_fields, NUM_FIELD_SIZE);
    ptr += NUM_FIELD_SIZE;

    for (int i = 0; i < tuple->num_fields; i++) {
        length += IS_ACTUAL_FIELD_SIZE;
        memcpy(ptr, &tuple->fields[i].is_actual, IS_ACTUAL_FIELD_SIZE);
        ptr += IS_ACTUAL_FIELD_SIZE;

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

    // Move the buffer content by 2 bytes
    memmove(buffer + 2, buffer, length);
    memcpy(buffer, &op_type, 1);
    memcpy(buffer + 1, &length, 1);

    return length + 2;
}

uint16_t serialize_alp_message(ALPMessage *alp_message, uint8_t *buffer) {
    uint8_t *ptr = buffer;

    Tuple *tuple = alp_message->tuple;

    uint16_t length = OP_TYPE_FIELD_SIZE; //alp message length in bytes, always starts with op_type
    memcpy(ptr, &alp_message->op_type, OP_TYPE_FIELD_SIZE);
    ptr += OP_TYPE_FIELD_SIZE;

    length += MSG_LENGTH_FIELD_SIZE;
    memcpy(ptr, &alp_message->msg_len, MSG_LENGTH_FIELD_SIZE);
    ptr += MSG_LENGTH_FIELD_SIZE;

    if (tuple != NULL) {
        length += NUM_FIELD_SIZE; //tuple length in bytes, always starts with num_fields
        memcpy(ptr, &tuple->num_fields, NUM_FIELD_SIZE);
        ptr += NUM_FIELD_SIZE;

        for (int i = 0; i < tuple->num_fields; i++) {
            length += IS_ACTUAL_FIELD_SIZE;
            memcpy(ptr, &tuple->fields[i].is_actual, IS_ACTUAL_FIELD_SIZE);
            ptr += IS_ACTUAL_FIELD_SIZE;
            
            length += TYPE_FIELD_SIZE;
            memcpy(ptr, &tuple->fields[i].type, TYPE_FIELD_SIZE);
            ptr += TYPE_FIELD_SIZE;
            
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
                memcpy(ptr, tuple->fields[i].data.string_field.value, field_length);
                ptr += field_length;
                length += field_length;
            }
        }
    }

    return length;
}

void deserialize_alp_message(uint8_t *buffer, ALPMessage *alp_message) {
    uint8_t *ptr = buffer;

    Tuple *tuple = (Tuple *)malloc(sizeof(Tuple));

    alp_message->op_type = *(uint8_t *)ptr;
    ptr += OP_TYPE_FIELD_SIZE;

    alp_message->msg_len = *(uint16_t *)ptr;
    ptr += MSG_LENGTH_FIELD_SIZE;

    if (tuple != NULL) {
        tuple->num_fields = *(uint8_t *)ptr;
        ptr += NUM_FIELD_SIZE;

        tuple->fields = (Field *)malloc(tuple->num_fields * sizeof(Field));

        for (int i = 0; i < tuple->num_fields; i++) {
            tuple->fields[i].is_actual = *(uint8_t *)ptr;
            ptr += IS_ACTUAL_FIELD_SIZE;

            tuple->fields[i].type = *(uint8_t *)ptr;
            ptr += TYPE_FIELD_SIZE;

            if (tuple->fields[i].type == TS_INT) {
                tuple->fields[i].data.int_field = *(uint16_t *)ptr;
                ptr += INT_FIELD_SIZE;
            } else if (tuple->fields[i].type == TS_FLOAT) {
                tuple->fields[i].data.float_field = *(float *)ptr;
                ptr += FLOAT_FIELD_SIZE;
            } else if (tuple->fields[i].type == TS_STRING) {
                uint8_t field_length = *(uint8_t *)ptr;
                tuple->fields[i].data.string_field.length = field_length;
                ptr += STRING_FIELD_LENGTH_SIZE;
                tuple->fields[i].data.string_field.value = (char *)malloc(field_length);
                strncpy(tuple->fields[i].data.string_field.value, (char *)ptr, field_length);
                ptr += field_length;
            }
        }
    }
    alp_message->tuple = tuple;
}

void deserialize_tuple(uint8_t *buffer, Tuple *tuple, ALPMessage *alp_message) {
    uint8_t *ptr = buffer;
    alp_message->op_type = *(uint8_t *)ptr;
    ptr += OP_TYPE_FIELD_SIZE;


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

void freeALPMessage(ALPMessage *alp_message) {
    if (alp_message->tuple != NULL) {
        for (int i = 0; i < alp_message->tuple->num_fields; ++i) {
            if (alp_message->tuple->fields[i].type == TS_STRING) {
                free(alp_message->tuple->fields[i].data.string_field.value);
            }
        }
        free(alp_message->tuple->fields);
        free(alp_message->tuple);
    }
    free(alp_message);
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
