#include "ddp_sender.h"
#include "log.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// DDP header, see http://www.3waylabs.com/ddp/
#define DDP_HEADER_LEN 10
#define DDP_FLAGS_VER1_PUSH 0x41 // version 1 (bits 6-7) + PUSH flag (bit 0)
#define DDP_TYPE_RGB24 0x01
#define DDP_ID_DEFAULT_OUTPUT 0x01
#define DDP_MAX_PAYLOAD 1400 // stay well under typical MTU, no fragmentation support needed yet

static int _sockfd = -1;
static struct sockaddr_in _dest;
static uint64_t _frames_sent = 0;
static bool _last_send_ok = false;

int ddp_sender_init(const char* host, int port)
{
    ddp_sender_close();

    _frames_sent = 0;
    _last_send_ok = false;

    _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (_sockfd < 0) {
        WARN("Could not create DDP UDP socket: %s", strerror(errno));
        return -1;
    }

    memset(&_dest, 0, sizeof(_dest));
    _dest.sin_family = AF_INET;
    _dest.sin_port = htons((uint16_t)port);

    if (inet_pton(AF_INET, host, &_dest.sin_addr) <= 0) {
        WARN("Invalid DDP target address: %s", host);
        close(_sockfd);
        _sockfd = -1;
        return -1;
    }

    return 0;
}

int ddp_sender_send(const uint8_t* led_rgb, size_t led_count)
{
    if (_sockfd < 0)
        return -1;

    size_t payload_len = led_count * 3;
    if (payload_len > DDP_MAX_PAYLOAD) {
        WARN("DDP payload too large for a single packet (%zu bytes), truncating", payload_len);
        payload_len = DDP_MAX_PAYLOAD - (DDP_MAX_PAYLOAD % 3);
    }

    uint8_t packet[DDP_HEADER_LEN + DDP_MAX_PAYLOAD];
    packet[0] = DDP_FLAGS_VER1_PUSH;
    packet[1] = 0; // sequence number, 0 = unused
    packet[2] = DDP_TYPE_RGB24;
    packet[3] = DDP_ID_DEFAULT_OUTPUT;
    packet[4] = 0; // channel offset (32-bit big endian), always 0
    packet[5] = 0;
    packet[6] = 0;
    packet[7] = 0;
    packet[8] = (uint8_t)((payload_len >> 8) & 0xFF);
    packet[9] = (uint8_t)(payload_len & 0xFF);
    memcpy(packet + DDP_HEADER_LEN, led_rgb, payload_len);

    ssize_t sent = sendto(_sockfd, packet, DDP_HEADER_LEN + payload_len, 0,
        (struct sockaddr*)&_dest, sizeof(_dest));
    if (sent < 0) {
        WARN("DDP sendto failed: %s", strerror(errno));
        _last_send_ok = false;
        return -1;
    }

    _frames_sent++;
    _last_send_ok = true;
    return 0;
}

void ddp_sender_close(void)
{
    if (_sockfd >= 0) {
        close(_sockfd);
        _sockfd = -1;
    }
}

uint64_t ddp_sender_frames_sent(void)
{
    return _frames_sent;
}

bool ddp_sender_last_send_ok(void)
{
    return _last_send_ok;
}
