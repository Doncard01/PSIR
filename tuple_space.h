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




int alp_send(int socket_fd, const char *dest_ip, const char *dest_port, Tuple *tuple, uint8_t type) {
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(dest_ip, dest_port, &hints, &res) != 0) {
        fprintf(stderr, "Error getting address info\n");
        return TS_FAIL;
    }

    ALPMessage alpMsg;
    alpMsg.op_type = type;
    alpMsg.tuple = tuple;

    uint8_t alpBuffer[BUFSIZE];
    uint16_t alpMsgLen = serialize_tuple(alpMsg.tuple, alpBuffer + TYPE_FIELD_SIZE);

    alpMsg.msg_len = alpMsgLen;
    memcpy(alpBuffer + TYPE_FIELD_SIZE, &alpMsg.msg_len, sizeof(alpMsg.msg_len));

        if (sendto(socket_fd, alpBuffer, alpMsgLen + TYPE_FIELD_SIZE, 0, res->ai_addr, res->ai_addrlen) == -1) {
        fprintf(stderr, "Error sending ALP message\n");
        freeaddrinfo(res);
        return TS_FAIL;
    }
    freeaddrinfo(res);
    return TS_SUCCESS;
}

int alp_receive(int socket_fd, const char* dest_port, TupleSpace *tSpace) {
    struct sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    uint8_t alpBuffer[BUFSIZE];

    ssize_t bytes_received = recvfrom(socket_fd, alpBuffer, BUFSIZE, 0, (struct sockaddr*)&sender_addr, &sender_addr_len);
    char sender_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(sender_addr.sin_addr), sender_ip, INET_ADDRSTRLEN);

    
    if (bytes_received == -1) {
        fprintf(stderr, "Error receiving ALP message\n");
        return TS_FAIL;
    }

    ALPMessage alpMsg;
    memcpy(&alpMsg.op_type, alpBuffer, TYPE_FIELD_SIZE);

    // Deserialize the tuple from the ALP message
    Tuple recvTuple;
    deserialize_tuple(alpBuffer + TYPE_FIELD_SIZE, &recvTuple);


    switch (alpMsg.op_type) {
        case ALP_OUT:
            ts_out(tSpace, &recvTuple);
            printf("Received ALP_OUT message. Added tuple to the tuple space.\n");
            break;

        case ALP_IN:
            pthread_mutex_lock(&tSpace->mutex);

            TupleSpaceEntry *matchingEntry = NULL;
            for (int i = 0; i < tSpace->count; ++i) {
                if (tuple_matches(&recvTuple, tSpace->entries[i].tuple)) {
                    matchingEntry = &tSpace->entries[i];
                       for (int j = i; j < tSpace->count - 1; ++j) {
                            tSpace->entries[j] = tSpace->entries[j + 1];
                        }
                tSpace->count--;

                break;
                }
            }

            if (matchingEntry != NULL) {
                // Found a matching tuple in the tuple space
                TupleSpaceEntry *responseEntry = (TupleSpaceEntry *)malloc(sizeof(TupleSpaceEntry));
                responseEntry->tuple = matchingEntry->tuple;
                responseEntry->count = matchingEntry->count;

                alp_send(socket_fd, sender_ip, dest_port, responseEntry, ALP_IN);


                printf("Received ALP_IN message. Sent matching tuple back to the same IP address.\n");
            } else {
                // No matching tuple found
                printf("Received ALP_IN message, but no matching tuple found in the tuple space.\n");
            }

            pthread_mutex_unlock(&tSpace->mutex);
            break;

        case ALP_ACK:
            break;

        case ALP_RD:
            pthread_mutex_lock(&tSpace->mutex);

            TupleSpaceEntry *matchingEntry2 = NULL;
            for (int i = 0; i < tSpace->count; ++i) {
                if (tuple_matches(&recvTuple, tSpace->entries[i].tuple)) {
                    matchingEntry2 = &tSpace->entries[i];
                break;
                }
            }

            if (matchingEntry2 != NULL) {
                // Found a matching tuple in the tuple space
                TupleSpaceEntry *responseEntry = (TupleSpaceEntry *)malloc(sizeof(TupleSpaceEntry));
                responseEntry->tuple = matchingEntry2->tuple;
                responseEntry->count = matchingEntry2->count;

                alp_send(socket_fd, sender_ip, dest_port, responseEntry, ALP_IN);


                printf("Received ALP_IN message. Sent matching tuple back to the same IP address.\n");
            } else {
                // No matching tuple found
                printf("Received ALP_IN message, but no matching tuple found in the tuple space.\n");
            }

            pthread_mutex_unlock(&tSpace->mutex);
            break;

        default:
            printf("Received message with unknown operation type: %d\n", alpMsg.op_type);
            break;
    }

    free_tuple(&recvTuple);

    return TS_SUCCESS;
}

int tuple_matches(Tuple *t1, Tuple *t2) {
    if (t1->num_fields != t2->num_fields) {
        return 0;  // Different number of fields, no match
    }

    for (int i = 0; i < t1->num_fields; ++i) {
        if (t1->fields[i].type != t2->fields[i].type) {
            return 0;  // Different field types, no match
        }

        switch (t1->fields[i].type) {
            case TS_INT:
                if (t1->fields[i].data.int_field != t2->fields[i].data.int_field) {
                    return 0;  // Field values are different, no match
                }
                break;

            case TS_FLOAT:
                if (t1->fields[i].data.float_field != t2->fields[i].data.float_field) {
                    return 0;  // Field values are different, no match
                }
                break;

            case TS_STRING:
                if (strcmp(t1->fields[i].data.string_field.value, t2->fields[i].data.string_field.value) != 0) {
                    return 0;  // Field values are different, no match
                }
                break;

            default:
                return 0;  // Unknown field type, no match
        }
    }

    return 1;  // All fields match
}


#endif
