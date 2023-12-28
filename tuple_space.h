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
#define TYPE_INT 0x01 // 0000 0001
#define TYPE_FLOAT 0x02 // 0000 0010
#define TYPE_STRING 0x04 // 0000 0100
#define INT_SIZE sizeof(int)
#define FLOAT_SIZE sizeof(float)
#define STRING_SIZE 1 // 1 byte

// ALP message structure
typedef struct {
    uint8_t app_id;
    uint8_t op_type;
    uint8_t number_of_fields;
    uint8_t *tuple;
} ALPMessage;


// Tuple field structure: | TYPE | DATA |
typedef struct {
    uint8_t data_type;
    void *data;
} Field;

// Tuple structure: (Field1, Field2, ...)
typedef struct {
    uint8_t num_fields;
    Field **fields;
} Tuple;