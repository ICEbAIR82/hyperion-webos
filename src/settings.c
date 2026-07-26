#include "settings.h"
#include <stdio.h>

void settings_init(settings_t* settings)
{
    settings->ui_backend = strdup("auto");
    settings->video_backend = strdup("auto");

    settings->address = strdup("172.16.24.1");
    settings->port = 19400;
    settings->priority = 150;
    settings->unix_socket = false;

    settings->fps = 30;
    settings->width = 320;
    settings->height = 180;
    settings->quirks = 0;

    settings->send_nv12 = false;
    settings->no_video = false;
    settings->no_gui = false;
    settings->autostart = false;
    settings->vsync = true;

    settings->no_hdr = false;
    settings->no_powerstate = false;

    settings->dump_frames = false;

    settings->direct_led_output = true;
    settings->wled_address = strdup("172.16.24.60");
    settings->wled_port = 4048;
    settings->smoothing_rise_time_ms = 200;
    settings->smoothing_fall_time_ms = 80;

    settings->letterbox_detection = true;
    settings->scene_dimming = false;

    settings->led_count = LED_LAYOUT_DEFAULT_COUNT;
    settings->led_layout = malloc(sizeof(led_region_t) * (size_t)settings->led_count);
    memcpy(settings->led_layout, led_layout_default, sizeof(led_region_t) * (size_t)settings->led_count);

    // Matches the reference layout above (led_layout_default) so a fresh
    // install's web UI generator starts from the same known-good example.
    settings->led_layout_top = 70;
    settings->led_layout_bottom = 48;
    settings->led_layout_left = 41;
    settings->led_layout_right = 41;
    settings->led_layout_gap_length = 22;
    settings->led_layout_gap_position = 124;
    settings->led_layout_input_position = 125;
    settings->led_layout_group_h = 0;
    settings->led_layout_group_v = 0;
    settings->led_layout_reverse = true;

    settings->wb_red = 100;
    settings->wb_green = 100;
    settings->wb_blue = 100;
}

int settings_load_json(settings_t* settings, jvalue_ref source)
{
    jvalue_ref value;

    if ((value = jobject_get(source, j_cstr_to_buffer("backend"))) && jis_string(value)) {
        free(settings->video_backend);
        raw_buffer str = jstring_get(value);
        settings->video_backend = strdup(str.m_str);
        jstring_free_buffer(str);
    }

    if ((value = jobject_get(source, j_cstr_to_buffer("uibackend"))) && jis_string(value)) {
        free(settings->ui_backend);
        raw_buffer str = jstring_get(value);
        settings->ui_backend = strdup(str.m_str);
        jstring_free_buffer(str);
    }

    if ((value = jobject_get(source, j_cstr_to_buffer("address"))) && jis_string(value)) {
        free(settings->address);
        raw_buffer str = jstring_get(value);
        settings->address = strdup(str.m_str);
        jstring_free_buffer(str);
    }
    if ((value = jobject_get(source, j_cstr_to_buffer("port"))) && jis_number(value))
        jnumber_get_i32(value, &settings->port);
    if ((value = jobject_get(source, j_cstr_to_buffer("priority"))) && jis_number(value))
        jnumber_get_i32(value, &settings->priority);
    if ((value = jobject_get(source, j_cstr_to_buffer("unix-socket"))) && jis_boolean(value))
        jboolean_get(value, &settings->unix_socket);

    if ((value = jobject_get(source, j_cstr_to_buffer("fps"))) && jis_number(value))
        jnumber_get_i32(value, &settings->fps);
    if ((value = jobject_get(source, j_cstr_to_buffer("width"))) && jis_number(value))
        jnumber_get_i32(value, &settings->width);
    if ((value = jobject_get(source, j_cstr_to_buffer("height"))) && jis_number(value))
        jnumber_get_i32(value, &settings->height);
    if ((value = jobject_get(source, j_cstr_to_buffer("nv12"))) && jis_boolean(value))
        jboolean_get(value, &settings->send_nv12);
    if ((value = jobject_get(source, j_cstr_to_buffer("quirks"))) && jis_number(value))
        jnumber_get_i32(value, &settings->quirks);

    if ((value = jobject_get(source, j_cstr_to_buffer("vsync"))) && jis_boolean(value))
        jboolean_get(value, &settings->vsync);
    if ((value = jobject_get(source, j_cstr_to_buffer("novideo"))) && jis_boolean(value))
        jboolean_get(value, &settings->no_video);
    if ((value = jobject_get(source, j_cstr_to_buffer("nogui"))) && jis_boolean(value))
        jboolean_get(value, &settings->no_gui);
    if ((value = jobject_get(source, j_cstr_to_buffer("autostart"))) && jis_boolean(value))
        jboolean_get(value, &settings->autostart);

    if ((value = jobject_get(source, j_cstr_to_buffer("nohdr"))) && jis_boolean(value))
        jboolean_get(value, &settings->no_hdr);
    if ((value = jobject_get(source, j_cstr_to_buffer("nopowerstate"))) && jis_boolean(value))
        jboolean_get(value, &settings->no_powerstate);

    if ((value = jobject_get(source, j_cstr_to_buffer("directledoutput"))) && jis_boolean(value))
        jboolean_get(value, &settings->direct_led_output);
    if ((value = jobject_get(source, j_cstr_to_buffer("wledaddress"))) && jis_string(value)) {
        free(settings->wled_address);
        raw_buffer str = jstring_get(value);
        settings->wled_address = strdup(str.m_str);
        jstring_free_buffer(str);
    }
    if ((value = jobject_get(source, j_cstr_to_buffer("wledport"))) && jis_number(value))
        jnumber_get_i32(value, &settings->wled_port);
    if ((value = jobject_get(source, j_cstr_to_buffer("smoothingrisetimems"))) && jis_number(value))
        jnumber_get_i32(value, &settings->smoothing_rise_time_ms);
    if ((value = jobject_get(source, j_cstr_to_buffer("smoothingfalltimems"))) && jis_number(value))
        jnumber_get_i32(value, &settings->smoothing_fall_time_ms);
    if ((value = jobject_get(source, j_cstr_to_buffer("letterboxdetection"))) && jis_boolean(value))
        jboolean_get(value, &settings->letterbox_detection);
    if ((value = jobject_get(source, j_cstr_to_buffer("scenedimming"))) && jis_boolean(value))
        jboolean_get(value, &settings->scene_dimming);

    if ((value = jobject_get(source, j_cstr_to_buffer("ledlayout"))) && jis_array(value)) {
        ssize_t region_count = jarray_size(value);
        led_region_t* regions = malloc(sizeof(led_region_t) * (size_t)region_count);

        for (ssize_t i = 0; i < region_count; i++) {
            jvalue_ref region = jarray_get(value, i);
            double hmin = 0, hmax = 0, vmin = 0, vmax = 0;
            jvalue_ref field;

            if ((field = jobject_get(region, j_cstr_to_buffer("hmin"))) && jis_number(field))
                jnumber_get_f64(field, &hmin);
            if ((field = jobject_get(region, j_cstr_to_buffer("hmax"))) && jis_number(field))
                jnumber_get_f64(field, &hmax);
            if ((field = jobject_get(region, j_cstr_to_buffer("vmin"))) && jis_number(field))
                jnumber_get_f64(field, &vmin);
            if ((field = jobject_get(region, j_cstr_to_buffer("vmax"))) && jis_number(field))
                jnumber_get_f64(field, &vmax);

            regions[i].hmin = (float)hmin;
            regions[i].hmax = (float)hmax;
            regions[i].vmin = (float)vmin;
            regions[i].vmax = (float)vmax;
        }

        free(settings->led_layout);
        settings->led_layout = regions;
        settings->led_count = (int)region_count;
    }

    if ((value = jobject_get(source, j_cstr_to_buffer("ledlayouttop"))) && jis_number(value))
        jnumber_get_i32(value, &settings->led_layout_top);
    if ((value = jobject_get(source, j_cstr_to_buffer("ledlayoutbottom"))) && jis_number(value))
        jnumber_get_i32(value, &settings->led_layout_bottom);
    if ((value = jobject_get(source, j_cstr_to_buffer("ledlayoutleft"))) && jis_number(value))
        jnumber_get_i32(value, &settings->led_layout_left);
    if ((value = jobject_get(source, j_cstr_to_buffer("ledlayoutright"))) && jis_number(value))
        jnumber_get_i32(value, &settings->led_layout_right);
    if ((value = jobject_get(source, j_cstr_to_buffer("ledlayoutgaplength"))) && jis_number(value))
        jnumber_get_i32(value, &settings->led_layout_gap_length);
    if ((value = jobject_get(source, j_cstr_to_buffer("ledlayoutgapposition"))) && jis_number(value))
        jnumber_get_i32(value, &settings->led_layout_gap_position);
    if ((value = jobject_get(source, j_cstr_to_buffer("ledlayoutinputposition"))) && jis_number(value))
        jnumber_get_i32(value, &settings->led_layout_input_position);
    if ((value = jobject_get(source, j_cstr_to_buffer("ledlayoutgrouph"))) && jis_number(value))
        jnumber_get_i32(value, &settings->led_layout_group_h);
    if ((value = jobject_get(source, j_cstr_to_buffer("ledlayoutgroupv"))) && jis_number(value))
        jnumber_get_i32(value, &settings->led_layout_group_v);
    if ((value = jobject_get(source, j_cstr_to_buffer("ledlayoutreverse"))) && jis_boolean(value))
        jboolean_get(value, &settings->led_layout_reverse);

    if ((value = jobject_get(source, j_cstr_to_buffer("wbred"))) && jis_number(value))
        jnumber_get_i32(value, &settings->wb_red);
    if ((value = jobject_get(source, j_cstr_to_buffer("wbgreen"))) && jis_number(value))
        jnumber_get_i32(value, &settings->wb_green);
    if ((value = jobject_get(source, j_cstr_to_buffer("wbblue"))) && jis_number(value))
        jnumber_get_i32(value, &settings->wb_blue);

    return 0;
}

int settings_save_json(settings_t* settings, jvalue_ref target)
{
    jobject_set(target, j_cstr_to_buffer("backend"), jstring_create(settings->video_backend));
    jobject_set(target, j_cstr_to_buffer("uibackend"), jstring_create(settings->ui_backend));

    jobject_set(target, j_cstr_to_buffer("address"), jstring_create(settings->address));
    jobject_set(target, j_cstr_to_buffer("port"), jnumber_create_i32(settings->port));
    jobject_set(target, j_cstr_to_buffer("priority"), jnumber_create_i32(settings->priority));
    jobject_set(target, j_cstr_to_buffer("unix-socket"), jboolean_create(settings->unix_socket));

    jobject_set(target, j_cstr_to_buffer("fps"), jnumber_create_i32(settings->fps));
    jobject_set(target, j_cstr_to_buffer("width"), jnumber_create_i32(settings->width));
    jobject_set(target, j_cstr_to_buffer("height"), jnumber_create_i32(settings->height));
    jobject_set(target, j_cstr_to_buffer("nv12"), jboolean_create(settings->send_nv12));
    jobject_set(target, j_cstr_to_buffer("quirks"), jnumber_create_i32(settings->quirks));

    jobject_set(target, j_cstr_to_buffer("vsync"), jboolean_create(settings->vsync));
    jobject_set(target, j_cstr_to_buffer("novideo"), jboolean_create(settings->no_video));
    jobject_set(target, j_cstr_to_buffer("nogui"), jboolean_create(settings->no_gui));
    jobject_set(target, j_cstr_to_buffer("autostart"), jboolean_create(settings->autostart));

    jobject_set(target, j_cstr_to_buffer("nohdr"), jboolean_create(settings->no_hdr));
    jobject_set(target, j_cstr_to_buffer("nopowerstate"), jboolean_create(settings->no_powerstate));

    jobject_set(target, j_cstr_to_buffer("directledoutput"), jboolean_create(settings->direct_led_output));
    jobject_set(target, j_cstr_to_buffer("wledaddress"), jstring_create(settings->wled_address));
    jobject_set(target, j_cstr_to_buffer("wledport"), jnumber_create_i32(settings->wled_port));
    jobject_set(target, j_cstr_to_buffer("smoothingrisetimems"), jnumber_create_i32(settings->smoothing_rise_time_ms));
    jobject_set(target, j_cstr_to_buffer("smoothingfalltimems"), jnumber_create_i32(settings->smoothing_fall_time_ms));
    jobject_set(target, j_cstr_to_buffer("letterboxdetection"), jboolean_create(settings->letterbox_detection));
    jobject_set(target, j_cstr_to_buffer("scenedimming"), jboolean_create(settings->scene_dimming));

    jvalue_ref led_layout_arr = jarray_create(NULL);
    for (int i = 0; i < settings->led_count; i++) {
        jvalue_ref region = jobject_create();
        jobject_set(region, j_cstr_to_buffer("hmin"), jnumber_create_f64(settings->led_layout[i].hmin));
        jobject_set(region, j_cstr_to_buffer("hmax"), jnumber_create_f64(settings->led_layout[i].hmax));
        jobject_set(region, j_cstr_to_buffer("vmin"), jnumber_create_f64(settings->led_layout[i].vmin));
        jobject_set(region, j_cstr_to_buffer("vmax"), jnumber_create_f64(settings->led_layout[i].vmax));
        jarray_append(led_layout_arr, region);
    }
    jobject_set(target, j_cstr_to_buffer("ledlayout"), led_layout_arr);

    jobject_set(target, j_cstr_to_buffer("ledlayouttop"), jnumber_create_i32(settings->led_layout_top));
    jobject_set(target, j_cstr_to_buffer("ledlayoutbottom"), jnumber_create_i32(settings->led_layout_bottom));
    jobject_set(target, j_cstr_to_buffer("ledlayoutleft"), jnumber_create_i32(settings->led_layout_left));
    jobject_set(target, j_cstr_to_buffer("ledlayoutright"), jnumber_create_i32(settings->led_layout_right));
    jobject_set(target, j_cstr_to_buffer("ledlayoutgaplength"), jnumber_create_i32(settings->led_layout_gap_length));
    jobject_set(target, j_cstr_to_buffer("ledlayoutgapposition"), jnumber_create_i32(settings->led_layout_gap_position));
    jobject_set(target, j_cstr_to_buffer("ledlayoutinputposition"), jnumber_create_i32(settings->led_layout_input_position));
    jobject_set(target, j_cstr_to_buffer("ledlayoutgrouph"), jnumber_create_i32(settings->led_layout_group_h));
    jobject_set(target, j_cstr_to_buffer("ledlayoutgroupv"), jnumber_create_i32(settings->led_layout_group_v));
    jobject_set(target, j_cstr_to_buffer("ledlayoutreverse"), jboolean_create(settings->led_layout_reverse));

    jobject_set(target, j_cstr_to_buffer("wbred"), jnumber_create_i32(settings->wb_red));
    jobject_set(target, j_cstr_to_buffer("wbgreen"), jnumber_create_i32(settings->wb_green));
    jobject_set(target, j_cstr_to_buffer("wbblue"), jnumber_create_i32(settings->wb_blue));

    return 0;
}

int settings_load_file(settings_t* settings, char* source)
{
    int ret = 0;

    JSchemaInfo schema;
    jvalue_ref parsed;

    FILE* f = fopen(source, "rb");
    // File read failed
    if (f == NULL) {
        return -1;
    }

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* json = calloc(fsize + 1, 1);
    fread(json, fsize, 1, f);
    fclose(f);

    jschema_info_init(&schema, jschema_all(), NULL, NULL);
    parsed = jdom_parse(j_cstr_to_buffer(json), DOMOPT_NOOPT, &schema);

    // Parsing failed
    if (jis_null(parsed)) {
        free(json);
        j_release(&parsed);
        return -2;
    }

    ret = settings_load_json(settings, parsed);

    free(json);
    j_release(&parsed);
    return ret;
}

int settings_save_file(settings_t* settings, char* target)
{
    jvalue_ref jobj = jobject_create();
    int ret = settings_save_json(settings, jobj);

    if (ret != 0) {
        j_release(&jobj);
        return -2;
    }

    FILE* fd = fopen(target, "wb");
    if (fd == NULL) {
        j_release(&jobj);
        return -1;
    }

    fputs(jvalue_tostring_simple(jobj), fd);
    fclose(fd);

    j_release(&jobj);
    return 0;
}
