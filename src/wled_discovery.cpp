#include "wled_discovery.h"
#include "log.h"

#include <httplib.h>
#include <pbnjson.h>

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>

#include <atomic>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace {

struct WledDevice {
    std::string ip;
    std::string name;
    int led_count;
};

std::mutex g_mutex;
std::vector<WledDevice> g_devices;
std::atomic<bool> g_scanning(false);
std::atomic<size_t> g_next_index(0);
std::vector<uint32_t> g_candidates; // candidate host addresses, network byte order

const int PROBE_TIMEOUT_US = 300 * 1000; // 300ms - keep the scan reasonably fast
const int PROBE_ONE_TIMEOUT_US = 1000 * 1000; // 1s - a single on-demand check can afford to wait longer
const uint32_t MAX_HOSTS_TO_SCAN = 512; // safety cap against scanning an unexpectedly huge subnet
const int SCAN_WORKER_COUNT = 24;

bool probe_wled(const std::string& ip, int timeout_us, std::string* out_name, int* out_led_count)
{
    httplib::Client cli(ip, 80);
    cli.set_connection_timeout(0, timeout_us);
    cli.set_read_timeout(0, timeout_us);

    auto res = cli.Get("/json/info");
    if (!res || res->status != 200) {
        return false;
    }

    JSchemaInfo schema;
    jschema_info_init(&schema, jschema_all(), NULL, NULL);
    jvalue_ref parsed = jdom_parse(j_cstr_to_buffer(res->body.c_str()), DOMOPT_NOOPT, &schema);
    if (jis_null(parsed)) {
        j_release(&parsed);
        return false;
    }

    bool ok = false;
    jvalue_ref value;

    if ((value = jobject_get(parsed, j_cstr_to_buffer("leds"))) && jis_object(value)) {
        jvalue_ref count_val;
        if ((count_val = jobject_get(value, j_cstr_to_buffer("count"))) && jis_number(count_val)) {
            int32_t count = 0;
            jnumber_get_i32(count_val, &count);
            *out_led_count = count;
            ok = true;
        }
    }

    if ((value = jobject_get(parsed, j_cstr_to_buffer("name"))) && jis_string(value)) {
        raw_buffer buf = jstring_get(value);
        *out_name = std::string(buf.m_str, buf.m_len);
        jstring_free_buffer(buf);
    } else {
        *out_name = ip;
    }

    j_release(&parsed);
    return ok;
}

void scan_worker()
{
    for (;;) {
        size_t idx = g_next_index.fetch_add(1);
        if (idx >= g_candidates.size()) {
            return;
        }

        struct in_addr addr;
        addr.s_addr = g_candidates[idx];
        std::string ip = inet_ntoa(addr);

        std::string name;
        int led_count = 0;
        if (probe_wled(ip, PROBE_TIMEOUT_US, &name, &led_count)) {
            std::lock_guard<std::mutex> lock(g_mutex);
            g_devices.push_back({ ip, name, led_count });
        }
    }
}

void scan_thread_main()
{
    std::vector<std::thread> workers;
    for (int i = 0; i < SCAN_WORKER_COUNT; i++) {
        workers.emplace_back(scan_worker);
    }
    for (auto& w : workers) {
        w.join();
    }
    g_scanning = false;
}

bool find_local_subnet(uint32_t* out_network, uint32_t* out_mask, uint32_t* out_self)
{
    struct ifaddrs* ifaddr;
    if (getifaddrs(&ifaddr) != 0) {
        return false;
    }

    bool found = false;
    for (struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET)
            continue;
        if (ifa->ifa_flags & IFF_LOOPBACK)
            continue;
        if (!(ifa->ifa_flags & IFF_UP))
            continue;
        if (!ifa->ifa_netmask)
            continue;

        uint32_t addr = ((struct sockaddr_in*)ifa->ifa_addr)->sin_addr.s_addr;
        uint32_t mask = ((struct sockaddr_in*)ifa->ifa_netmask)->sin_addr.s_addr;

        *out_self = addr;
        *out_mask = mask;
        *out_network = addr & mask;
        found = true;
        break;
    }

    freeifaddrs(ifaddr);
    return found;
}

} // namespace

void wled_discovery_start(void)
{
    bool expected = false;
    if (!g_scanning.compare_exchange_strong(expected, true)) {
        return; // already scanning
    }

    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_devices.clear();
    }

    uint32_t network, mask, self;
    if (!find_local_subnet(&network, &mask, &self)) {
        WARN("wled_discovery: could not determine local subnet");
        g_scanning = false;
        return;
    }

    uint32_t host_count = ntohl(~mask) + 1;
    if (host_count > MAX_HOSTS_TO_SCAN) {
        WARN("wled_discovery: subnet too large to scan (%u hosts), skipping", host_count);
        g_scanning = false;
        return;
    }

    g_candidates.clear();
    g_next_index = 0;

    uint32_t network_host_order = ntohl(network);
    for (uint32_t i = 1; i + 1 < host_count; i++) { // skip network (.0) and broadcast (.255)
        uint32_t candidate = htonl(network_host_order + i);
        if (candidate == self)
            continue; // skip our own address
        g_candidates.push_back(candidate);
    }

    INFO("wled_discovery: scanning %zu candidate hosts", g_candidates.size());

    std::thread(scan_thread_main).detach();
}

char* wled_discovery_get_results(void)
{
    jvalue_ref jobj = jobject_create();
    jobject_set(jobj, j_cstr_to_buffer("scanning"), jboolean_create(g_scanning));

    jvalue_ref arr = jarray_create(NULL);
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        for (const auto& dev : g_devices) {
            jvalue_ref item = jobject_create();
            jobject_set(item, j_cstr_to_buffer("ip"), jstring_create(dev.ip.c_str()));
            jobject_set(item, j_cstr_to_buffer("name"), jstring_create(dev.name.c_str()));
            jobject_set(item, j_cstr_to_buffer("ledCount"), jnumber_create_i32(dev.led_count));
            jarray_append(arr, item);
        }
    }
    jobject_set(jobj, j_cstr_to_buffer("devices"), arr);

    char* result = strdup(jvalue_tostring_simple(jobj));
    j_release(&jobj);
    return result;
}

char* wled_discovery_probe_one(const char* ip)
{
    std::string name;
    int led_count = 0;
    bool reachable = probe_wled(ip, PROBE_ONE_TIMEOUT_US, &name, &led_count);

    jvalue_ref jobj = jobject_create();
    jobject_set(jobj, j_cstr_to_buffer("reachable"), jboolean_create(reachable));
    if (reachable) {
        jobject_set(jobj, j_cstr_to_buffer("name"), jstring_create(name.c_str()));
        jobject_set(jobj, j_cstr_to_buffer("ledCount"), jnumber_create_i32(led_count));
    }

    char* result = strdup(jvalue_tostring_simple(jobj));
    j_release(&jobj);
    return result;
}
