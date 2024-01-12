#ifndef TUPLE_SPACE_H
#define TUPLE_SPACE_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// Operation types
#define ALP_OUT 0x01  // 0000 0001
#define ALP_IN  0x02  // 0000 0010
#define ALP_INP 0x04  // 0000 0100
#define ALP_RD  0x08  // 0000 1000
#define ALP_RDP 0x10  // 0001 0000
#define ALP_ACK 0x20  // 0010 0000

// Data types
#define TS_INT 0
#define TS_FLOAT 1
#define INT_SIZE sizeof(int)
#define FLOAT_SIZE sizeof(float)
#define YES 1
#define NO 0
#define TS_SUCCESS 1
#define TS_FAIL 0

// ALP message structure
typedef struct {
    uint8_t op_type;
    uint16_t msg_len;
    Tuple *tuple;
} ALPMessage;


// Tuple field structure: | TYPE | DATA |
typedef struct {
    uint8_t is_actual; // YES or NO
    uint8_t type; // TS_INT or TS_FLOAT
    union {
        int int_field;
        float float_field;
    } data;
} Field;

// Tuple structure: (Field1, Field2, ...)
typedef struct {
    uint8_t num_fields;
    Field **fields;
} Tuple;

// Tuple space structure
typedef struct {
    Tuple **tuples;
    uint8_t num_tuples;
} TupleSpace;

// Implementation in .c files
uint8_t ts_out(char* tuple_name, Field* fields, int num_fields);
uint8_t ts_in(char* tuple_name, Field* fields, int num_fields);
uint8_t ts_inp(char* tuple_name, Field* fields, int num_fields);
uint8_t ts_rd(char* tuple_name, Field* fields, int num_fields);
uint8_t ts_rdp(char* tuple_name, Field* fields, int num_fields);


#endif