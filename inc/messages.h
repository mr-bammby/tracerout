/**
 * @file messages.h
 * @brief Network packet structures and I/O layout configurations.
 * @details Defines explicit memory layouts for custom IP, ICMP, and UDP network layers 
 *          tailored for a 64-bit Linux target environment.
 * @date 2026-05-28
 * @author Domen Banfi
 */

#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include "main.h"

/** @brief Size definitions for network headers */
#define IP_HEADER_SIZE 20
#define ICMP_HEADER_SIZE 8
#define UDP_HEADER_SIZE 8


/** @brief Probe receipt tracking states */
#define PROBE_NOT_RECEIVED 0
#define PROBE_RECEIVED     1
#define PROBE_PASSIVE      2

/**
 * @struct ip_header_t
 * @brief IPv4 header structure with explicit packet bitfields.
 */
typedef struct __attribute__((packed)) {
    uint8_t header_len :4;
    uint8_t ver :4;
    uint8_t dscp :6; /* priority class [default 0] */
    uint8_t ecn :2;  /* congestion signal [default 0] */
    uint16_t total_len;
    uint16_t id;     /* used for defragmentation */
    uint16_t flags :3; /* fragmentation settings */
    uint16_t fragment_off :13; /* used for defragmentation */
    uint8_t ttl; /* time to life (counter) */
    uint8_t protocol; /* default: IPPROTO_ICMP */
    uint16_t chksum;
    struct in_addr ip_src;
    struct in_addr ip_dest;
} ip_header_t;

/**
 * @struct roh_echo_req_t
 * @brief ICMP Echo request/reply Rest-of-Header format identifier and sequence layout.
 */
typedef struct {
    uint16_t id;
    uint16_t seq_num;
} roh_echo_req_t;

/**
 * @struct icmp_message_t
 * @brief Layout representing a full ICMP message structure.
 */
typedef struct {
    uint8_t  type;
    uint8_t  code;
    uint16_t chksum;
    uint32_t roh; 
    uint8_t data[56]; /* Payload size for our traceroute (adjusted as needed) */
} icmp_message_t;

/**
 * @struct udp_message_t
 * @brief Layout representing a complete UDP transmission message with payload.
 */
typedef struct {
    uint16_t    src_port;
    uint16_t    dst_port;
    uint16_t    len;
    uint16_t    chksum;
    uint8_t     data[32]; /* Payload size for our traceroute (adjusted as needed) */
} udp_message_t;

int receive_responses(const network_context_t *net_ctx, stats_t *stats);
int send_next_message(const network_context_t *net_ctx, uint8_t ttl, const udp_message_t *udp_message, stats_t *stats, uint32_t probe_index);

#endif // __MESSAGES_H__