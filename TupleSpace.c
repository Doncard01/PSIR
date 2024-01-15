#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <pthread.h>
#include <stdint.h>
#include "tuple_space.h"

#define MY_IP       "169.254.97.104"
#define PORT_S1     "9545"
#define MAX_BUFF    1472 //UDP payload size
#define KEY_LENGTH 10

int socket_fd, pos;
struct addrinfo h, *p;
struct addrinfo *r=NULL;
struct sockaddr_in c;
unsigned char received[MAX_BUFF];
unsigned char sent[MAX_BUFF];
unsigned char mip_str[INET_ADDRSTRLEN];
int c_len=(sizeof(c));

typedef struct {
    uint16_t key;  // Identifier for the tuple, random
    Tuple *tuple;   // Tuple value
} TupleSpaceEntry;

typedef struct {
    TupleSpaceEntry entries[MAX_TUPLES];
    int count;
    pthread_mutex_t mutex;
} TupleSpace;

TupleSpace tSpace;
pthread_mutex_t tSpaceMutex = PTHREAD_MUTEX_INITIALIZER;

uint16_t generateRandomKey() {
    srand(time(NULL));
    return (uint16_t)rand();
}

char* getCurrentTime() {
    static char timestamp[20];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    strftime(timestamp, sizeof(timestamp)-1, "[%Y-%m-%d %H:%M]%S", t);

    return timestamp;
}

int tuple_matches(Tuple *pattern, Tuple *checked) {
    if (pattern->num_fields != checked->num_fields) {
        return NO;
    }
    for (int i = 0; i < pattern->num_fields; ++i) {
        if (pattern->fields[i].type != checked->fields[i].type) {
            return NO;
        }
        if (pattern->fields[i].is_actual) {
            if (pattern->fields[i].type == TS_INT && pattern->fields[i].data.int_field != checked->fields[i].data.int_field) {
                return NO;
            } else if (pattern->fields[i].type == TS_FLOAT && pattern->fields[i].data.float_field != checked->fields[i].data.float_field) {
                return NO;
            } else if (pattern->fields[i].type == TS_STRING && strcmp(pattern->fields[i].data.string_field.value, checked->fields[i].data.string_field.value) != 0) {
                return NO;
            }
        }
    }
    return YES;  // Wszystkie pola pasują
}

void send_alp_message(uint8_t op_type, TupleSpaceEntry *entry) {
    uint8_t *buffer = (uint8_t *)malloc(ALP_MESSAGE_SIZE * sizeof(uint8_t));
    memset(buffer, 0, ALP_MESSAGE_SIZE * sizeof(uint8_t));

    ALPMessage *alp_message = (ALPMessage *)malloc(ALP_MESSAGE_SIZE * sizeof(uint8_t));
    alp_message->op_type = op_type;
    alp_message->tuple = entry->tuple;
    alp_message->msg_len = serialize_tuple(NULL, buffer, ALP_ACK);
    if((pos=sendto(socket_fd, buffer, alp_message->msg_len, 0, (const struct sockaddr *)&c, c_len )) < 0) {
        fprintf(stderr, "%s ERROR: %s (%s:%d)\n", getCurrentTime(), strerror(errno), __FILE__, __LINE__);   
    } else {
        printf("%s Sent ALP message, op_type: %d.\n", getCurrentTime(), op_type);
    }
    freeALPMessage(alp_message);
    free(buffer);
}

void freeTupleSpaceEntry(TupleSpaceEntry *entry) {
    if (entry->tuple != NULL) {
        for (int i = 0; i < entry->tuple->num_fields; ++i) {
            if (entry->tuple->fields[i].type == TS_STRING) {
                free(entry->tuple->fields[i].data.string_field.value);
            }
        }
        free(entry->tuple->fields);
        free(entry->tuple);
    }
    free(entry);
}

void remove_tuple(TupleSpace *tSpace, TupleSpaceEntry *entry) {
    int index = -1;
    for (int i = 0; i < tSpace->count; ++i) {
        if (&tSpace->entries[i] == entry) {
            index = i;
            break;
        }
    }
    if (index != -1) {
        // Shift the remaining entries to fill the gap
        for (int i = index; i < tSpace->count - 1; ++i) {
            tSpace->entries[i] = tSpace->entries[i + 1];
        }
        tSpace->count--;
        freeTupleSpaceEntry(entry);
    }
}


int find_tuple(TupleSpace *tSpace, Tuple *pattern, TupleSpaceEntry **matchingEntry) {
    for (int i = 0; i < tSpace->count; ++i) {
        if (tuple_matches(pattern, tSpace->entries[i].tuple)) {
            *matchingEntry = &tSpace->entries[i];
            return TS_SUCCESS;
        }
    }
    return TS_FAIL;
}

int ts_out(TupleSpace *tSpace, Tuple *tuple) {
    pthread_mutex_lock(&tSpaceMutex);

    if (tSpace->count >= MAX_TUPLES) {
        pthread_mutex_unlock(&tSpaceMutex);
        send_alp_message(ALP_ERROR, NULL);
        return TS_FAIL;  // Tuple space is full
    }
    printf("Tuple %s added to Tuple Space.\n", tupleToString(tuple));
    send_alp_message(ALP_SUCCESS, NULL);

    TupleSpaceEntry *entry = &tSpace->entries[tSpace->count++];
    entry->tuple = tuple;
    entry->key = generateRandomKey();
    pthread_mutex_unlock(&tSpaceMutex);

    return TS_SUCCESS;
}

int ts_inp(TupleSpace *tSpace, Tuple *pattern) {
    pthread_mutex_lock(&tSpaceMutex);

    TupleSpaceEntry *matchingEntry = NULL;
    if (find_tuple(tSpace, pattern, &matchingEntry) == TS_SUCCESS) {
        // Found a matching tuple in the tuple space
        Tuple *tuple = matchingEntry->tuple;
        remove_tuple(tSpace, matchingEntry);
        pthread_mutex_unlock(&tSpaceMutex);
        printf("Received ALP_IN message. Sent matching tuple back to the same IP address.\n");
        send_alp_message(ALP_SUCCESS, matchingEntry);
        return TS_SUCCESS;
    } else {
        // No matching tuple found
        printf("Received ALP_IN message, but no matching tuple found in the tuple space.\n");
        send_alp_message(ALP_ERROR, NULL);
        pthread_mutex_unlock(&tSpaceMutex);
        return TS_FAIL;
    }
}

int ts_rdp(TupleSpace *tSpace, Tuple *pattern) {
    pthread_mutex_lock(&tSpaceMutex);

    TupleSpaceEntry *matchingEntry = NULL;
    if (find_tuple(tSpace, pattern, &matchingEntry) == TS_SUCCESS) {
        // Found a matching tuple in the tuple space
        Tuple *tuple = matchingEntry->tuple;
        pthread_mutex_unlock(&tSpaceMutex);
        printf("Received ALP_RDP message. Sent matching tuple back to the same IP address.\n");
        send_alp_message(ALP_SUCCESS, matchingEntry);
        return TS_SUCCESS;
    } else {
        // No matching tuple found
        printf("Received ALP_RDP message, but no matching tuple found in the tuple space.\n");
        send_alp_message(ALP_ERROR, NULL);
        pthread_mutex_unlock(&tSpaceMutex);
        return TS_FAIL;
    }
}

void *socket_listen(void *arg) {
    for(;;) {
        memset(&received, 0, sizeof(received));
        printf("%s Waiting for message...\n", getCurrentTime());
        pos=recvfrom(socket_fd, received, MAX_BUFF, 0, (struct sockaddr*)&c, &c_len );
        if(pos < 0) {
            fprintf(stderr, "%s ERROR: %s (%s:%d)\n", getCurrentTime(), strerror(errno), __FILE__, __LINE__);
            return TS_FAIL;
        } else {
            printf("%s Received message from %s:%d\n", getCurrentTime(), inet_ntoa(c.sin_addr), ntohs(c.sin_port));
            send_alp_message(ALP_ACK, NULL);
        }
        
        ALPMessage *alp_message = (ALPMessage *)malloc(ALP_MESSAGE_SIZE * sizeof(uint8_t));
        deserialize_alp_message(received, alp_message);
        Tuple *received_tuple = alp_message->tuple;
        printTuple(received_tuple);

    switch (alp_message->op_type) {
        case ALP_OUT:
            ts_out(&tSpace, received_tuple);
            printf("Received ALP_OUT message. Added tuple to the tuple space.\n");
            break;

        case ALP_INP:
            ts_inp(&tSpace, received_tuple);
            printf("Received ALP_INP message. Sent matching tuple back to the same IP address.\n");
            break;
        case ALP_ACK:
            break;
        case ALP_RDP:
            ts_rdp(&tSpace, received_tuple);
            printf("Received ALP_RDP message. Sent matching tuple back to the same IP address.\n");
            break;
        default:
            printf("Received message with unknown operation type: %d\n", alp_message->op_type);
            break;
    }
        freeALPMessage(alp_message);

    }
    return NULL;
}


int main() {
    TupleSpace* tSpace = (TupleSpace*)malloc(sizeof(TupleSpace));
    tSpace->count = 0;
    tSpace->mutex = tSpaceMutex;

    pthread_mutex_init(&tSpace->mutex, NULL);
    // Inicjalizacja TupleSpaceEntries
    for (int i = 0; i < MAX_TUPLES; i++) {
        tSpace->entries[i].tuple = NULL;
        tSpace->entries[i].key = 0;
    }

    memset(&h, 0, sizeof(struct addrinfo));
    memset(&c, 0, sizeof(c));
    memset(&mip_str, 0, sizeof(mip_str));

    h.ai_family = AF_INET; // Use IPv4
    h.ai_socktype = SOCK_DGRAM; // Datagram UDP
    h.ai_flags = AI_PASSIVE;

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 5000;

    //Opening socket
    if(getaddrinfo(MY_IP, PORT_S1, &h, &r)==0) {
            for(p=r; p!=NULL; p=p->ai_next) {
                struct sockaddr_in *mip=(struct sockaddr_in*)p->ai_addr;
                if(inet_ntop(p->ai_family, &(mip->sin_addr), mip_str, INET_ADDRSTRLEN)!=NULL) 
                printf("%s TupleSpace has IP: %s\n", getCurrentTime(), mip_str);
            }
        }

    socket_fd = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
    if(socket_fd==-1) {
        fprintf(stderr, "%s ERROR: %s (%s:%d)\n", getCurrentTime(), strerror(errno), __FILE__, __LINE__);
        printf("%s Closing application...\n", getCurrentTime());
        freeaddrinfo(r);
        exit(-1);
    }
    printf("%s Socket created successfully!\n", getCurrentTime());

    if(bind(socket_fd, r->ai_addr, r->ai_addrlen)!=0) {
        fprintf(stderr, "%s ERROR: %s (%s:%d)\n", getCurrentTime(), strerror(errno), __FILE__, __LINE__);
        printf("%s Closing application...\n", getCurrentTime());
        close(socket_fd);
        freeaddrinfo(r);
        exit(-1);
    }
    printf("%s Socket binded successfully!\n", getCurrentTime());

    pthread_t thread;
    pthread_create(&thread, NULL, socket_listen, NULL);

    for(;;) {
    }



    for (int i = 0; i < tSpace->count; i++) {
        freeTupleSpaceEntry(&tSpace->entries[i]);
    }
    pthread_mutex_destroy(&tSpace->mutex);
    free(tSpace);
    close(socket_fd);
    freeaddrinfo(r);

    return 0;
}
