#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int ddp_sender_init(const char* host, int port);
int ddp_sender_send(const uint8_t* led_rgb, size_t led_count);
void ddp_sender_close(void);

// Diagnostics for the status API
uint64_t ddp_sender_frames_sent(void);
bool ddp_sender_last_send_ok(void);
