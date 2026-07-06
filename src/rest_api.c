#include "rest_api.h"
#include "log.h"

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

char* rest_api_start(void)
{
    if (!_service)
        return strdup("{\"returnValue\":false}");

    bool ok = service_start(_service) == 0;
    char* result = malloc(32);
    snprintf(result, 32, "{\"returnValue\":%s}", ok ? "true" : "false");
    return result;
}

char* rest_api_stop(void)
{
    if (!_service)
        return strdup("{\"returnValue\":false}");

    bool ok = service_stop(_service) == 0;
    char* result = malloc(32);
    snprintf(result, 32, "{\"returnValue\":%s}", ok ? "true" : "false");
    return result;
}
