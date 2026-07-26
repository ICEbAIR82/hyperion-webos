#include "log_server.h"
#include "log.h"

#include <pbnjson.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define RING_CAPACITY 2000
#define TEXT_MAX_LEN 400
#define WIRE_LINE_MAX (TEXT_MAX_LEN + 48)
#define MAX_CLIENTS 4
#define MAX_LINES_PER_POLL 500
// A fresh TCP client should drain a backlog send almost instantly on a LAN;
// this just bounds how long a stalled client can hold up its own backlog
// replay (other clients/log_printf callers are never blocked by this).
#define BACKLOG_SEND_RETRY_LIMIT 40
#define BACKLOG_SEND_RETRY_SLEEP_US 50000

typedef struct {
    uint64_t seq;
    double elapsed_seconds;
    LogLevel level;
    char text[TEXT_MAX_LEN];
} log_entry_t;

static pthread_mutex_t _lock = PTHREAD_MUTEX_INITIALIZER;
static log_entry_t _ring[RING_CAPACITY];
static uint64_t _next_seq = 1;
static uint64_t _ring_count = 0;

static int _clients[MAX_CLIENTS];
static char _boot_id[40];

static int _listen_fd = -1;
static pthread_t _accept_thread;
static volatile bool _running = false;

static const char* level_str(LogLevel level)
{
    switch (level) {
    case Debug:
        return "DBG";
    case Info:
        return "INFO";
    case Warning:
        return "WARN";
    default:
        return "ERR";
    }
}

static void format_line(char* out, size_t out_size, const log_entry_t* e)
{
    snprintf(out, out_size, "%10.3fs [%-4s] %s\n", e->elapsed_seconds, level_str(e->level), e->text);
}

// Must be called with _lock held. Never blocks: a slow/stalled client just
// misses this line rather than stalling every other logging call site.
static void broadcast_locked(const char* line, size_t len)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (_clients[i] < 0)
            continue;
        ssize_t sent = send(_clients[i], line, len, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (sent < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            close(_clients[i]);
            _clients[i] = -1;
        }
    }
}

static void send_all_best_effort(int fd, const char* data, size_t len)
{
    size_t sent_total = 0;
    int retries = 0;
    while (sent_total < len && retries < BACKLOG_SEND_RETRY_LIMIT) {
        ssize_t n = send(fd, data + sent_total, len - sent_total, MSG_NOSIGNAL | MSG_DONTWAIT);
        if (n > 0) {
            sent_total += (size_t)n;
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            retries++;
            usleep(BACKLOG_SEND_RETRY_SLEEP_US);
            continue;
        }
        break; // real error - give up on the rest of the backlog
    }
}

static void* accept_loop(void* arg)
{
    (void)arg;

    while (_running) {
        struct sockaddr_in peer;
        socklen_t peer_len = sizeof(peer);
        int fd = accept(_listen_fd, (struct sockaddr*)&peer, &peer_len);
        if (fd < 0) {
            if (_running)
                WARN("log_server: accept() failed: %s", strerror(errno));
            continue;
        }

        INFO("log_server: client connected (%s)", inet_ntoa(peer.sin_addr));

        int flags = fcntl(fd, F_GETFL, 0);
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);

        // Snapshot the backlog and replay it to the new client *before*
        // registering it for live broadcast (below) - that way it can never
        // race with broadcast_locked() writing to the same fd concurrently
        // from another thread. The only cost is that lines logged during the
        // (normally near-instant) replay itself are missed by this client.
        pthread_mutex_lock(&_lock);
        uint64_t count = _ring_count;
        uint64_t start_seq = _next_seq - count;
        char* backlog = count > 0 ? malloc(count * WIRE_LINE_MAX + 1) : NULL;
        size_t backlog_len = 0;
        if (backlog) {
            for (uint64_t s = start_seq; s < _next_seq; s++) {
                char line[WIRE_LINE_MAX];
                format_line(line, sizeof(line), &_ring[s % RING_CAPACITY]);
                size_t l = strlen(line);
                memcpy(backlog + backlog_len, line, l);
                backlog_len += l;
            }
        }
        pthread_mutex_unlock(&_lock);

        if (backlog) {
            send_all_best_effort(fd, backlog, backlog_len);
            free(backlog);
        }

        pthread_mutex_lock(&_lock);
        int slot = -1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (_clients[i] < 0) {
                slot = i;
                break;
            }
        }
        if (slot >= 0)
            _clients[slot] = fd;
        pthread_mutex_unlock(&_lock);

        if (slot < 0) {
            WARN("log_server: too many clients already connected, closing new connection");
            close(fd);
        }
    }

    return NULL;
}

void log_server_init(uint16_t tcp_port)
{
    if (_running) {
        WARN("log_server_init() called twice, ignoring");
        return;
    }

    for (int i = 0; i < MAX_CLIENTS; i++)
        _clients[i] = -1;

    snprintf(_boot_id, sizeof(_boot_id), "%d-%lx", (int)getpid(), (unsigned long)time(NULL));

    _listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_listen_fd < 0) {
        ERR("log_server: socket() failed: %s", strerror(errno));
        return;
    }

    int opt = 1;
    setsockopt(_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(tcp_port);

    if (bind(_listen_fd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        ERR("log_server: bind() to port %d failed: %s", tcp_port, strerror(errno));
        close(_listen_fd);
        _listen_fd = -1;
        return;
    }

    if (listen(_listen_fd, 4) != 0) {
        ERR("log_server: listen() failed: %s", strerror(errno));
        close(_listen_fd);
        _listen_fd = -1;
        return;
    }

    _running = true;

    if (pthread_create(&_accept_thread, NULL, accept_loop, NULL) != 0) {
        ERR("log_server: failed to start accept thread");
        _running = false;
        close(_listen_fd);
        _listen_fd = -1;
        return;
    }

    INFO("log_server: listening on 0.0.0.0:%d (boot id %s)", tcp_port, _boot_id);
}

void log_server_stop(void)
{
    if (!_running)
        return;

    _running = false;

    if (_listen_fd >= 0) {
        shutdown(_listen_fd, SHUT_RDWR);
        close(_listen_fd);
        _listen_fd = -1;
    }

    pthread_join(_accept_thread, NULL);

    pthread_mutex_lock(&_lock);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (_clients[i] >= 0) {
            close(_clients[i]);
            _clients[i] = -1;
        }
    }
    pthread_mutex_unlock(&_lock);
}

void log_server_push(LogLevel level, double elapsed_seconds, const char* module, const char* message)
{
    if (!_running)
        return;

    log_entry_t entry;
    entry.elapsed_seconds = elapsed_seconds;
    entry.level = level;
    snprintf(entry.text, sizeof(entry.text), "%s: %s", module, message);

    char line[WIRE_LINE_MAX];
    format_line(line, sizeof(line), &entry);
    size_t line_len = strlen(line);

    pthread_mutex_lock(&_lock);
    entry.seq = _next_seq++;
    _ring[entry.seq % RING_CAPACITY] = entry;
    if (_ring_count < RING_CAPACITY)
        _ring_count++;
    broadcast_locked(line, line_len);
    pthread_mutex_unlock(&_lock);
}

char* log_server_build_json_since(uint64_t since_seq, const char* client_boot_id)
{
    pthread_mutex_lock(&_lock);

    bool same_boot = client_boot_id && client_boot_id[0] != '\0' && strcmp(client_boot_id, _boot_id) == 0;
    uint64_t oldest_seq = _next_seq - _ring_count;
    // since_seq is the last seq the caller already has, so lines start
    // strictly after it.
    uint64_t from_seq = same_boot ? since_seq + 1 : 0;
    if (from_seq < oldest_seq)
        from_seq = oldest_seq;

    uint64_t count = _next_seq - from_seq;
    if (count > MAX_LINES_PER_POLL)
        count = MAX_LINES_PER_POLL;
    uint64_t start = _next_seq - count;

    jvalue_ref lines = jarray_create(NULL);
    for (uint64_t s = start; s < _next_seq; s++) {
        log_entry_t* e = &_ring[s % RING_CAPACITY];
        jvalue_ref item = jobject_create();
        jobject_set(item, j_cstr_to_buffer("seq"), jnumber_create_i64((int64_t)e->seq));
        jobject_set(item, j_cstr_to_buffer("level"), jstring_create(level_str(e->level)));
        jobject_set(item, j_cstr_to_buffer("t"), jnumber_create_f64(e->elapsed_seconds));
        jobject_set(item, j_cstr_to_buffer("text"), jstring_create(e->text));
        jarray_append(lines, item);
    }

    jvalue_ref jobj = jobject_create();
    jobject_set(jobj, j_cstr_to_buffer("bootId"), jstring_create(_boot_id));
    jobject_set(jobj, j_cstr_to_buffer("latestSeq"), jnumber_create_i64((int64_t)(_next_seq - 1)));
    jobject_set(jobj, j_cstr_to_buffer("lines"), lines);

    pthread_mutex_unlock(&_lock);

    char* result = strdup(jvalue_tostring_simple(jobj));
    j_release(&jobj);
    return result;
}
