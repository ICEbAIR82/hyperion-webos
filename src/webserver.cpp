#include "webserver.h"
#include "log.h"
#include "rest_api.h"

#include <httplib.h>

#include <cstdlib>
#include <memory>
#include <thread>

namespace {
std::unique_ptr<httplib::Server> g_server;
std::thread g_thread;
} // namespace

int webserver_start(uint16_t port)
{
    if (g_server) {
        WARN("Webserver already running");
        return -1;
    }

    g_server = std::make_unique<httplib::Server>();

    g_server->set_mount_point("/", "/media/developer/apps/usr/palm/services/org.webosbrew.piccapplus.service/webui");

    g_server->Get("/api/config", [](const httplib::Request&, httplib::Response& res) {
        char* body = rest_api_get_config();
        res.set_content(body, "application/json");
        free(body);
    });

    g_server->Put("/api/config", [](const httplib::Request& req, httplib::Response& res) {
        char* body = nullptr;
        int status = rest_api_put_config(req.body.c_str(), &body);
        res.status = status;
        res.set_content(body, "application/json");
        free(body);
    });

    g_server->Get("/api/status", [](const httplib::Request&, httplib::Response& res) {
        char* body = rest_api_get_status();
        res.set_content(body, "application/json");
        free(body);
    });

    g_server->Post("/api/start", [](const httplib::Request&, httplib::Response& res) {
        char* body = rest_api_start();
        res.set_content(body, "application/json");
        free(body);
    });

    g_server->Post("/api/stop", [](const httplib::Request&, httplib::Response& res) {
        char* body = rest_api_stop();
        res.set_content(body, "application/json");
        free(body);
    });

    g_server->Post("/api/wled/discover", [](const httplib::Request&, httplib::Response& res) {
        rest_api_wled_discover_start();
        res.set_content("{\"started\":true}", "application/json");
    });

    g_server->Get("/api/wled/discover", [](const httplib::Request&, httplib::Response& res) {
        char* body = rest_api_wled_discover_results();
        res.set_content(body, "application/json");
        free(body);
    });

    g_server->Get("/api/wled/info", [](const httplib::Request& req, httplib::Response& res) {
        std::string ip = req.get_param_value("ip");
        char* body = rest_api_wled_info(ip.c_str());
        res.set_content(body, "application/json");
        free(body);
    });

    g_thread = std::thread([port]() {
        INFO("Webserver listening on 0.0.0.0:%d", port);
        if (!g_server->listen("0.0.0.0", port)) {
            ERR("Webserver failed to bind to port %d", port);
        }
    });

    return 0;
}

void webserver_stop(void)
{
    if (!g_server) {
        return;
    }

    g_server->stop();
    if (g_thread.joinable()) {
        g_thread.join();
    }
    g_server.reset();
}
