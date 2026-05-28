/**
 * @file main.h
 * @brief Core structures and constants for ft_traceroute.
 * @details Defines network contexts, statistics tracking structures, and global configuration limits.
 * @date 2026-05-29
 * @author Domen Banfi
 */

#ifndef FT_MAIN_H
#define FT_MAIN_H

#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <stdint.h>

/** @brief Global application definitions */
#define NAME "ft_traceroute"
#define VERSION "1.0"

/** @brief Traceroute operation constraints */
#define UDP_PAYLOAD_STR "@ABCDEFGHIJKLMNOPQRSTUVWXYZABCDE"
#define UDP_TRANSMIT_PAYLOAD_SIZE (sizeof(UDP_PAYLOAD_STR) - 1)
#define TRANSMIT_MESSAGE_SIZE (IP_HEADER_SIZE + UDP_HEADER_SIZE + UDP_TRANSMIT_PAYLOAD_SIZE)
#define TRACEROUTE_PORT_BASE 33434
#define PROBE_NUM 3u
#define MAX_TTL 30u
#define MIN_TTL 1u

/**
 * @struct probe_stats_t
 * @brief Tracking metrics for an individual UDP probe transmission.
 */
typedef struct {
    struct timeval send_time;        /**< System time when probe was transmitted */
    struct timeval recv_time;        /**< System time when ICMP response was received */
    char *dest_addr_str;             /**< Allocated string of responding router IP */
    uint8_t received;                /**< State flag: 0 (not received), 1 (received), 2 (passive), macros in message.h */
} probe_stats_t;

/**
 * @struct stats_t
 * @brief Aggregated statistics for a single TTL hop batch.
 */
typedef struct {
    uint32_t transmit_cnt;           /**< Number of probes successfully dispatched */
    uint32_t receive_cnt;            /**< Number of valid ICMP responses processed */
    uint16_t dest_base_port;         /**< Starting UDP destination port for the current hop */
    uint8_t target_reached;          /**< Boolean flag indicating destination host replied with ICMP with destination unreachable */
    probe_stats_t probes[PROBE_NUM]; /**< Tracking blocks for individual batch probes */
} stats_t;

/**
 * @struct network_context_t
 * @brief Encapsulates all active file descriptors and target routing state.
 */
typedef struct {
    int sock_udp;                    /**< File descriptor for UDP transmission socket */
    int sock_icmp;                   /**< File descriptor for raw ICMP listening socket */
    fd_set read_fds;                 /**< Multiplexing set for non-blocking I/O monitoring */
    struct sockaddr_in dst;          /**< Resolved IPv4 destination address structure */
    socklen_t dstlen;                /**< Byte length of the destination address structure */
} network_context_t;

#endif /* FT_MAIN_H */