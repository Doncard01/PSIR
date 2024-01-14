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

