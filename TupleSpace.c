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
#include "tuple_space.h"

#define MY_IP       "169.254.97.104"
#define PORT_S1     "9545"
#define MAX_BUFF    1472 //UDP payload size

int socket_fd;
struct addrinfo h, *p;
struct addrinfo *r=NULL;
struct sockaddr_in c;
unsigned char received[MAX_BUFF];
unsigned char sent[MAX_BUFF];
unsigned char mip_str[INET_ADDRSTRLEN];
int c_len=(sizeof(c));


typedef struct {
    char key[50];  // Identifier for the tuple
    Tuple *tuple;   // Tuple value
    int count;
} TupleSpaceEntry;

typedef struct {
    TupleSpaceEntry entries[MAX_TUPLES];
    int count;
    pthread_mutex_t mutex;
} TupleSpace;
// --- TUPLE SPACE


int ts_out(TupleSpace *tSpace, Tuple *tuple) {
    pthread_mutex_lock(&tSpace->mutex);

    if (tSpace->count >= MAX_TUPLES) {
        pthread_mutex_unlock(&tSpace->mutex);
        return TS_FAIL;  // Tuple space is full
    }
    printf("Dodano tuple do tuple_space\n");
    TupleSpaceEntry *entry = &tSpace->entries[tSpace->count++];
    entry->tuple = tuple;
    entry->count = 1;

    pthread_mutex_unlock(&tSpace->mutex);

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


char* getCurrentTime() {
    static char timestamp[20]; // Buffer for the timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    strftime(timestamp, sizeof(timestamp)-1, "[%Y-%m-%d %H:%M]%S", t);

    return timestamp;
}

void *socket_listen(void *arg) {
    for(;;) {
        memset(&received, 0, sizeof(received));
        printf("%s Waiting for message...\n", getCurrentTime());
        int pos=recvfrom(socket_fd, received, MAX_BUFF, 0, (struct sockaddr*)&c, &c_len );
        if(pos < 0) {
            fprintf(stderr, "%s ERROR: %s (%s:%d)\n", getCurrentTime(), strerror(errno), __FILE__, __LINE__);
            return TS_FAIL;
        }
        
        printf("%s Received message: ", getCurrentTime());
        ALPMessage *alp_message = (ALPMessage *)malloc(ALP_MESSAGE_SIZE * sizeof(uint8_t));
        deserialize_alp_message(received, alp_message);
        tupleToString(alp_message->tuple);

        /*
        uint8_t op_type = received[0]; na razie leci sama krotka
        if (op_type != ALP_ACK) {
            Tuple *recv_tuple = (Tuple *)malloc(sizeof(Tuple));
            recv_tuple->fields = (Field *)malloc(recv_tuple->num_fields * sizeof(Field));
        } else {       
            switch (op_type)
            {
            case ALP_OUT:
                ts_out(&myTupleSpace, &myTuple);
                break;

            case ALP_IN:
                ts_in(&myTupleSpace, &myTuple);
                break;

            ...itd.
            
            default:
                printf("Received message with unknown operation type.\n");
                break;
            }
        }

        if op_type != ALP_ACK
        */
        freeALPMessage(alp_message);

    }
    return NULL;
}


int main() {
    memset(&h, 0, sizeof(struct addrinfo));

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

    close(socket_fd);
    freeaddrinfo(r);

    return 0;
}

