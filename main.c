#include <inttypes.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/select.h>

typedef struct {
    struct timespec start;
    char *dest_addr_str;
    uint32_t transmit_cnt;
    uint32_t receive_cnt;
    float min;
    float max;
    float avg;
    float mean_sqr;
} stats_t;

typedef struct __attribute__((packed)) {
    uint8_t header_len :4;
    uint8_t ver :4;
    uint8_t dscp :6; //priority class [default 0]
    uint8_t ecn :2;  //congestion signal [default 0]
    uint16_t total_len;
    uint16_t id;     //used for defragmentation
    uint16_t flags :3; //fragmentation settings
    uint16_t fragment_off :13; //used for defragmentation
    uint8_t ttl; // time to life (counter)
    uint8_t protocol; // default: IPPROTO_ICMP
    uint16_t chksum;
    struct in_addr ip_src;
    struct in_addr ip_dest;
} ip_header_t;

/* ICMP Echo request/reply identifier+sequence */
typedef struct {
    uint16_t id;
    uint16_t seq_num;
} roh_echo_req_t;

/* Full ICMP message */
typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t chksum;
    uint32_t roh;  // identifier + sequence number
    uint8_t data[56];  // payload (ping typically uses 56 bytes)
} icmp_message_t;

/* Full UDP message */
typedef struct {
    uint16_t    src_port;
    uint16_t    dst_port;
    uint16_t    len;
    uint16_t    chksum;
    uint8_t data[32];  // payload (ping typically uses 56 bytes)
} udp_message_t;


uint16_t checksum(const void *data, uint32_t len, uint16_t start_val) {
    const uint16_t *p = data;
    uint32_t sum = start_val;

    // Sum 16-bit words
    while (len > 1) {
        sum += *p++;
        len -= 2;
    }

    // If there's a leftover byte, pad with zero
    if (len == 1) {
        uint16_t last = 0;
        *(uint8_t *)&last = *(const uint8_t *)p;
        sum += last;
    }

    // Fold 32-bit sum to 16 bits (carry wraparound)
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // One’s complement
    return (uint16_t)~sum;
}

void print_bytes(const void *addr, uint32_t len) {
    const uint8_t *p = (const uint8_t *)addr;
    for (int i = 0; i < len; i++) {
        printf("%02x ", p[i]);
    }
    printf("\n\n");
}

int get_src_ip_for_dest(const char *dest_ip, char *out, size_t out_len) {
    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return -1;

    struct sockaddr_in dst = {0};
    dst.sin_family = AF_INET;
    inet_pton(AF_INET, dest_ip, &dst.sin_addr);
    dst.sin_port = htons(53);   // arbitrary (DNS)

    // this does NOT send anything — it just triggers routing lookup
    connect(s, (struct sockaddr *)&dst, sizeof(dst));

    struct sockaddr_in local = {0};
    socklen_t len = sizeof(local);
    getsockname(s, (struct sockaddr *)&local, &len);

    inet_ntop(AF_INET, &local.sin_addr, out, out_len);

    close(s);
    return 0;
}

float elapsed_ms(const struct timespec *start, const struct timespec *end) {
    long sec  = end->tv_sec  - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;
    return ((float)sec * 1000.0 + (float)nsec / 1000000.0);
}

stats_t g_stat = {0};

int main (void) {
    ip_header_t ip_header = {0};
    udp_message_t udp_message = {0};
    struct timespec start, end;

    char ip[INET_ADDRSTRLEN];
    get_src_ip_for_dest("8.8.8.8", ip, sizeof(ip));
    printf("Source IP would be: %s\n", ip);

    ip_header.ver = 4;
    ip_header.header_len = 5;  // 5 * 4bytes
    ip_header.ttl = 2;  // TTL 1 for testing, it will expire immediately
    ip_header.protocol = 17;
    ip_header.ip_dest.s_addr = inet_addr("8.8.8.8");
    ip_header.ip_src.s_addr = inet_addr(ip);

    ip_header.total_len = htons(20 + sizeof(udp_message_t));  // IP header size
    ip_header.chksum = checksum(&ip_header, sizeof(ip_header), 0);

    udp_message.src_port = htons(35073);
    udp_message.dst_port = htons(33434);
    udp_message.len = htons(sizeof(udp_message_t));
    memcpy(&(udp_message.data), "@ABCDEFGHIJKLMNOPQRSTUVWXYZABCDE", 32);
    udp_message.chksum = checksum(&udp_message, sizeof(udp_message_t), 0);

    int sock_udp, sock_icmp;

    uint8_t buf[1500];

    // Create a raw socket for ICMP
    sock_udp = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_udp < 0) {
        perror("socket");
        exit(1);
    }

    int one = 1;
    if (setsockopt(sock_udp, IPPROTO_IP, IP_TTL, &ip_header.ttl, sizeof(ip_header.ttl)) < 0) {
        perror("setsockopt");
        return 1;
    }

    sock_icmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock_icmp < 0) {
        perror("socket");
        exit(1);
    }

    // if (setsockopt(sock_icmp, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
    //     perror("setsockopt");
    //     return 1;
    // }

    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;

    // if (bind(sock_icmp, (struct sockaddr*)&server, sizeof(server)) < 0) {
    //     perror("Bind failed");
    //     close(sock_icmp);
    //     exit(EXIT_FAILURE);
    // }

    fd_set read_fds;         // Set of file descriptors to monitor
    FD_ZERO(&read_fds);      // Initialize the set
    FD_SET(sock_icmp, &read_fds); // Add server socket to set

    int max_fd = sock_icmp;  // The highest-numbered file descriptor
    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_addr.s_addr = ip_header.ip_dest.s_addr;
    socklen_t dstlen = sizeof(dst);
    uint8_t packet[1500];
    memcpy(packet, &ip_header, sizeof(ip_header_t));
    memcpy(packet + sizeof(ip_header_t), &udp_message, sizeof(udp_message_t));

    print_bytes(packet, sizeof(ip_header));
    if (sendto(sock_udp, (void *)(packet + sizeof(ip_header_t)), sizeof(udp_message_t), 0, (struct sockaddr *)(&dst), sizeof(dst)) < 0) {
        perror("sendto");
    } else {
        printf("Sent IP header\n");
    }

    // Timeout setup
    struct timeval timeout;
    timeout.tv_sec = 5;  // Set timeout (5 seconds)
    timeout.tv_usec = 0;

    fd_set ready_fds = read_fds;  // Copy the set because select() modifies it

    // Wait for any socket to become ready (non-blocking)
    int activity = select(max_fd + 1, &ready_fds, NULL, NULL, &timeout);

    if (activity < 0) {
        perror("select() failed");
    } else if (activity == 0) {
        // Timeout, no activity
        printf("Timeout occurred! No data received.\n");
    } else {
        // Check if the server socket is ready for accepting connections
        if (FD_ISSET(sock_icmp, &ready_fds)) {
            int n = recvfrom(sock_icmp, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&dst, &dstlen);
            if (n < 0) {
                perror("recvfrom failed");
                close(sock_icmp);
                exit(EXIT_FAILURE);
            }
            buf[n] = '\0';  // Null-terminate the received data
            print_bytes(buf, n);

            // Here, you can process the ICMP message
            // Check the ICMP message type (e.g., Time Exceeded, Echo Reply, etc.)
            printf("Received ICMP response\n");
        }
    }
    close(sock_udp);
    close(sock_icmp);
    return 0;
}
