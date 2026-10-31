#include "rest_api.h"
#include "log.h"
#include "log_server.h"
#include "wled_discovery.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static service_t* _service = NULL;

void rest_api_init(service_t* service)
{
    _service = service;
}

char* rest_api_get_config(void)
{
    if (!_service)
        return strdup("{}");

    jvalue_ref jobj = service_build_settings_json(_service);
    char* result = strdup(jvalue_tostring_simple(jobj));
    j_release(&jobj);
    return result;
}

char* rest_api_get_status(void)
{
    if (!_service)
        return strdup("{}");

    jvalue_ref jobj = service_build_status_json(_service);
    char* result = strdup(jvalue_tostring_simple(jobj));
    j_release(&jobj);
    return result;
}

char* rest_api_get_live_leds(void)
{
    if (!_service)
        return strdup("{\"returnValue\":false,\"leds\":[]}");

    jvalue_ref jobj = service_build_live_leds_json(_service);
    char* result = strdup(jvalue_tostring_simple(jobj));
    j_release(&jobj);
    return result;
}

int rest_api_put_config(const char* body, char** out_response)
{
    if (!_service) {
        *out_response = strdup("{\"error\":\"service not initialized\"}");
        return 500;
    }

    JSchemaInfo schema;
    jschema_info_init(&schema, jschema_all(), NULL, NULL);
    jvalue_ref parsed = jdom_parse(j_cstr_to_buffer(body), DOMOPT_NOOPT, &schema);

    if (jis_null(parsed)) {
        j_release(&parsed);
        WARN("PUT /api/config: invalid JSON body");
        *out_response = strdup("{\"error\":\"invalid JSON\"}");
        return 400;
    }

    int res = service_apply_settings_json(_service, parsed);
    j_release(&parsed);

    if (res != 0) {
        WARN("PUT /api/config: failed to apply settings (%d)", res);
        *out_response = strdup("{\"error\":\"failed to apply settings\"}");
        return 400;
    }

    *out_response = rest_api_get_config();
    return 200;
}

char* rest_api_put_whitebalance_live(const char* body)
{
    if (!_service)
        return strdup("{\"returnValue\":false}");

    JSchemaInfo schema;
    jschema_info_init(&schema, jschema_all(), NULL, NULL);
    jvalue_ref parsed = jdom_parse(j_cstr_to_buffer(body), DOMOPT_NOOPT, &schema);

    if (jis_null(parsed)) {
        j_release(&parsed);
        WARN("PUT /api/whitebalance: invalid JSON body");
        return strdup("{\"returnValue\":false}");
    }

    settings_t* settings = _service->settings;
    int wb_red = settings->wb_red;
    int wb_green = settings->wb_green;
    int wb_blue = settings->wb_blue;
    double led_gamma = settings->led_gamma;
    int led_brightness = settings->led_brightness;

    jvalue_ref value;
    if ((value = jobject_get(parsed, j_cstr_to_buffer("wbred"))) && jis_number(value))
        jnumber_get_i32(value, &wb_red);
    if ((value = jobject_get(parsed, j_cstr_to_buffer("wbgreen"))) && jis_number(value))
        jnumber_get_i32(value, &wb_green);
    if ((value = jobject_get(parsed, j_cstr_to_buffer("wbblue"))) && jis_number(value))
        jnumber_get_i32(value, &wb_blue);
    if ((value = jobject_get(parsed, j_cstr_to_buffer("ledgamma"))) && jis_number(value))
        jnumber_get_f64(value, &led_gamma);
    if ((value = jobject_get(parsed, j_cstr_to_buffer("ledbrightness"))) && jis_number(value))
        jnumber_get_i32(value, &led_brightness);

    j_release(&parsed);

    service_apply_whitebalance_live(_service, wb_red, wb_green, wb_blue, led_gamma, led_brightness);
    return strdup("{\"returnValue\":true}");
}

char* rest_api_calibrate_start(const char* body)
{
    if (!_service)
        return strdup("{\"returnValue\":false}");

    JSchemaInfo schema;
    jschema_info_init(&schema, jschema_all(), NULL, NULL);
    jvalue_ref parsed = jdom_parse(j_cstr_to_buffer(body), DOMOPT_NOOPT, &schema);

    const char* color = "#ffffff";
    raw_buffer color_buf = { 0 };
    bool have_color_buf = false;

    if (!jis_null(parsed)) {
        jvalue_ref value = jobject_get(parsed, j_cstr_to_buffer("color"));
        if (value && jis_string(value)) {
            color_buf = jstring_get(value);
            color = color_buf.m_str;
            have_color_buf = true;
        }
    }

    service_start_calibration(_service, color);

    if (have_color_buf)
        jstring_free_buffer(color_buf);
    j_release(&parsed);

    return strdup("{\"returnValue\":true}");
}

char* rest_api_calibrate_stop(void)
{
    if (!_service)
        return strdup("{\"returnValue\":false}");

    service_stop_calibration(_service);
    return strdup("{\"returnValue\":true}");
}

char* rest_api_start(void)
{
    if (!_service)
        return strdup("{\"returnValue\":false}");

    // 0 = transitioned to running, 1 = was already running - both are success.
    bool ok = service_start(_service) >= 0;
    char* result = malloc(32);
    snprintf(result, 32, "{\"returnValue\":%s}", ok ? "true" : "false");
    return result;
}

char* rest_api_stop(void)
{
    if (!_service)
        return strdup("{\"returnValue\":false}");

    bool ok = service_stop(_service) >= 0;
    char* result = malloc(32);
    snprintf(result, 32, "{\"returnValue\":%s}", ok ? "true" : "false");
    return result;
}

void rest_api_wled_discover_start(void)
{
    wled_discovery_start();
}

char* rest_api_wled_discover_results(void)
{
    return wled_discovery_get_results();
}

char* rest_api_wled_info(const char* ip)
{
    const char* target = (ip && ip[0] != '\0') ? ip : (_service ? _service->settings->wled_address : NULL);
    if (!target) {
        return strdup("{\"reachable\":false}");
    }

    char* raw = wled_discovery_probe_one(target);

    JSchemaInfo schema;
    jschema_info_init(&schema, jschema_all(), NULL, NULL);
    jvalue_ref parsed = jdom_parse(j_cstr_to_buffer(raw), DOMOPT_NOOPT, &schema);
    free(raw);

    if (jis_null(parsed)) {
        j_release(&parsed);
        return strdup("{\"reachable\":false}");
    }

    if (_service) {
        jobject_set(parsed, j_cstr_to_buffer("configuredLedCount"), jnumber_create_i32(_service->settings->led_count));
    }

    char* result = strdup(jvalue_tostring_simple(parsed));
    j_release(&parsed);
    return result;
}

char* rest_api_get_logs(const char* since, const char* boot)
{
    uint64_t since_seq = (since && since[0] != '\0') ? strtoull(since, NULL, 10) : 0;
    return log_server_build_json_since(since_seq, boot);
}
