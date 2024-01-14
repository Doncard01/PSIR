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

#define MY_IP       "192.168.0.73"
#define PORT_S1     "9545"
#define PORT_S2     "24246"
#define MAX_BUFF    1472 //UDP payload size

int socket_fd; //deskryptory gniazd, trzeba otworzyc 1 do nasluchiwania i 1 do wysylania
// int socket_fd2;
struct addrinfo h, *p;
struct addrinfo h2, *p2;
struct addrinfo *r=NULL, *r2=NULL;
struct sockaddr_in c;
unsigned char received[MAX_BUFF], received2[MAX_BUFF];
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
        printf("%s Listening...\n", getCurrentTime());
        int pos=recvfrom(socket_fd, received, MAX_BUFF, 0, (struct sockaddr*)&c, &c_len );
        
        
    
        if(pos < 0) {
            fprintf(stderr, "%s ERROR: %s (%s:%d)\n", getCurrentTime(), strerror(errno), __FILE__, __LINE__);
            exit(-1);
        }
        

        //TEST ODBIERANIA KROTKI
        Tuple *recvTuple = (Tuple *)malloc(sizeof(Tuple));
        deserialize_tuple(received, recvTuple);
        tupleToString(recvTuple);
        
        if(alp_receive(socket_fd, PORT_S1, recvTuple) == TS_SUCCESS){
        printf("Message receive\n");
        }
        // printf("%s Received from (%s:%d): %s\n", getCurrentTime(), inet_ntoa(c.sin_addr),ntohs(c.sin_port),received);

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
        free_tuple(recvTuple);

    }
    return NULL;
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

