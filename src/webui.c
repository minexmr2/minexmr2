/*
Copyright (c) 2018, The Monero Project

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <pthread.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/listener.h>

#ifdef P2POOL
#include <json-c/json.h>
#endif

#include "log.h"
#include "pool.h"
#include "webui.h"

extern unsigned char webui_html[];
extern unsigned int webui_html_len;

static pthread_t handle;
static struct event_base *webui_base;
static struct evhttp *webui_httpd;
static struct evhttp_bound_socket *webui_listener;

static const char*
fetch_wa_cookie(struct evhttp_request *req)
{
    struct evkeyvalq *hdrs_in = evhttp_request_get_input_headers(req);
    const char *cookies = evhttp_find_header(hdrs_in, "Cookie");
    char *wa = NULL;

    if (cookies)
    {
        wa = strstr(cookies, "wa=");
        if (wa)
        {
            char *sc = strchr(wa, ';');
            if (sc)
                *sc = 0;
            wa += 3;
        }
    }
    return wa;
}

static void
send_json_workers(struct evhttp_request *req, void *arg)
{
    struct evbuffer *buf = evhttp_request_get_output_buffer(req);
    struct evkeyvalq *hdrs_out = NULL;
    char rig_list[0x40000] = {0};
    char *end = rig_list + sizeof(rig_list);
    const char *wa = fetch_wa_cookie(req);

    if (wa)
        worker_list(rig_list, end, wa);

    evbuffer_add_printf(buf, "[%s]", rig_list);
    hdrs_out = evhttp_request_get_output_headers(req);
    evhttp_add_header(hdrs_out, "Content-Type", "application/json");
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
}
#ifdef P2POOL
static const char* load_file_as_c_string(const char *filename)
{
    char *buffer = 0;
    long length;
    FILE *f = fopen(filename, "rb");
    if (f)
    {
      fseek(f, 0, SEEK_END);
      length = ftell(f);
      fseek(f, 0, SEEK_SET);
      buffer = malloc(length + 1);
      if (buffer)
      {
        fread(buffer, 1, length, f);
      }
      fclose(f);
    }
    if (buffer)
    {
        buffer[length] = 0;
        return buffer;
    }
    return NULL;
}
#endif
static void
send_json_stats(struct evhttp_request *req, void *arg)
{
    struct evbuffer *buf = evhttp_request_get_output_buffer(req);
    wui_context_t *context = (wui_context_t*) arg;
    struct evkeyvalq *hdrs_out = NULL;
    uint64_t ph = context->pool_stats->pool_hashrate;
#ifdef P2POOL
    uint64_t p2h = 0;
#endif
    uint64_t nh = context->pool_stats->network_hashrate;
    uint64_t nd = context->pool_stats->network_difficulty;
    uint64_t height = context->pool_stats->network_height;
    uint64_t ltf = context->pool_stats->last_template_fetched;
    uint64_t lbf = context->pool_stats->last_block_found;
    uint64_t lbfh = context->pool_stats->last_block_found_height;
    uint32_t pbf = context->pool_stats->pool_blocks_found;
#ifdef P2POOL
    const char* str_json_pool = load_file_as_c_string("/home/p2pool/stats/pool/stats");
    if (str_json_pool)
    {
        json_object *root = json_tokener_parse(str_json_pool);
        if (root)
        {
            json_object *pool_statistics = NULL;
            json_object_object_get_ex(root, "pool_statistics", &pool_statistics);
            if (pool_statistics)
            {
                json_object *hashRate = NULL;
                json_object_object_get_ex(pool_statistics, "hashRate", &hashRate);
                if (hashRate)
                {
                    if (json_object_is_type(hashRate, json_type_int))
                    {
                        p2h = (uint64_t)json_object_get_int64(hashRate);
                    }
                }
                json_object *lastBlockFound = NULL;
                json_object_object_get_ex(pool_statistics, "lastBlockFound", &lastBlockFound);
                if (lastBlockFound)
                {
                    if (json_object_is_type(lastBlockFound, json_type_int))
                    {
                        lbfh = (uint64_t)json_object_get_int64(lastBlockFound);
                    }
                }
                json_object *lastBlockFoundTime = NULL;
                json_object_object_get_ex(pool_statistics, "lastBlockFoundTime", &lastBlockFoundTime);
                if (lastBlockFoundTime)
                {
                    if (json_object_is_type(lastBlockFoundTime, json_type_int))
                    {
                        lbf = (uint64_t)json_object_get_int64(lastBlockFoundTime);
                    }
                }
                json_object *totalBlocksFound = NULL;
                json_object_object_get_ex(pool_statistics, "totalBlocksFound", &totalBlocksFound);
                if (totalBlocksFound)
                {
                    if (json_object_is_type(totalBlocksFound, json_type_int))
                    {
                        pbf = (uint64_t)json_object_get_int64(totalBlocksFound);
                    }
                }
            }
        }
        free(str_json_pool);
    }
#endif
    uint64_t rh = context->pool_stats->round_hashes;
    unsigned ss = context->allow_self_select;
    double mh[6] = {0};
    double mb = 0.0;
    uint64_t wc = 0;
    const char *wa = fetch_wa_cookie(req);
    double mlpval = 0.0;
    uint64_t mlpts = 0;
    uint64_t mhrmean24h = 0;

    if (wa)
    {
        account_hr(mh, wa);
        wc = worker_count(wa);
        uint64_t balance = account_balance(wa);
        mb = (double) balance / 1000000000000.0;
        uint64_t payamount = 0;
        int rc = get_last_payout(wa, &payamount, &mlpts);
        if (rc == 0)
            mlpval = (double) payamount / 1000000000000.0;
        uint64_t hrtmp = 0;
        rc = get_24h_meanstddev_hr(wa, &hrtmp, NULL);
        if (rc == 0)
            mhrmean24h = hrtmp;
    }

    evbuffer_add_printf(buf, "{"
            "\"pool_hashrate\":%"PRIu64","
#ifdef P2POOL
            "\"p2pool_hashrate\":%"PRIu64","
#endif
            "\"round_hashes\":%"PRIu64","
            "\"network_hashrate\":%"PRIu64","
            "\"network_difficulty\":%"PRIu64","
            "\"network_height\":%"PRIu64","
            "\"last_template_fetched\":%"PRIu64","
            "\"last_block_found\":%"PRIu64","
            "\"last_block_found_height\":%"PRIu64","
            "\"pool_blocks_found\":%d,"
            "\"payment_threshold\":%g,"
            "\"pool_fee\":%g,"
            "\"pool_port\":%d,"
            "\"pool_ssl_port\":%d,"
            "\"allow_self_select\":%u,"
            "\"connected_miners\":%d,"
            "\"miner_hashrate\":%"PRIu64","
            "\"miner_hashrate_stats\":["
                    "%"PRIu64",%"PRIu64",%"PRIu64","
                    "%"PRIu64",%"PRIu64",%"PRIu64"],"
            "\"miner_balance\":%.8f,"
            "\"miner_last_payout\":%.8f,"
            "\"miner_last_payout_ts\":%"PRIu64","
            "\"miner_hashrate_mean24h\":%"PRIu64","
            "\"worker_count\": %"PRIu64
            "}",
            ph,
#ifdef P2POOL
            p2h,
#endif
            rh, nh, nd, height, ltf, lbf, lbfh, pbf,
            context->payment_threshold, context->pool_fee,
            context->pool_port, context->pool_ssl_port,
            ss, context->pool_stats->connected_accounts,
            (uint64_t)mh[0],
            (uint64_t)mh[0], (uint64_t)mh[1], (uint64_t)mh[2],
            (uint64_t)mh[3], (uint64_t)mh[4], (uint64_t)mh[5], mb, mlpval, mlpts, mhrmean24h, wc);
    hdrs_out = evhttp_request_get_output_headers(req);
    evhttp_add_header(hdrs_out, "Content-Type", "application/json");
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
}

static void
process_request(struct evhttp_request *req, void *arg)
{
    const char *url = evhttp_request_get_uri(req);
    struct evbuffer *buf = NULL;
    struct evkeyvalq *hdrs_out = NULL;

    if (strstr(url, "/stats") != NULL)
    {
        send_json_stats(req, arg);
        return;
    }

    if (strstr(url, "/workers") != NULL)
    {
        send_json_workers(req, arg);
        return;
    }

    buf = evhttp_request_get_output_buffer(req);
    evbuffer_add(buf, webui_html, webui_html_len);
    hdrs_out = evhttp_request_get_output_headers(req);
    evhttp_add_header(hdrs_out, "Content-Type", "text/html");
    evhttp_send_reply(req, HTTP_OK, "OK", buf);
}

static void *
thread_main(void *ctx)
{
    wui_context_t *context = (wui_context_t*) ctx;
    struct evconnlistener *lev = NULL;
    struct addrinfo *info = NULL;
    int rc;
    char port[6] = {0};
    sprintf(port, "%d", context->port);
    if ((rc = getaddrinfo(context->listen, port, 0, &info)))
    {
        log_error("Error parsing listen address: %s", gai_strerror(rc));
        return 0;
    }
    lev = evconnlistener_new_bind(webui_base, 0, NULL,
            LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE | LEV_OPT_REUSEABLE_PORT,
            -1, (struct sockaddr*)info->ai_addr, info->ai_addrlen);
    if (!lev)
    {
        log_error("%s", strerror(errno));
        return 0;
    }
    webui_listener = evhttp_bind_listener(webui_httpd, lev);
    if(!webui_listener)
    {
        log_error("Failed to bind for port: %u", context->port);
        return 0;
    }
    evhttp_set_gencb(webui_httpd, process_request, ctx);
    event_base_dispatch(webui_base);
    event_base_free(webui_base);
    return 0;
}

int
start_web_ui(wui_context_t *context)
{
    log_info("Starting Web UI on %s:%d", context->listen, context->port);
    if (webui_base || handle)
    {
        log_error("Already running");
        return -1;
    }
    webui_base = event_base_new();
    if (!webui_base)
    {
        log_error("Failed to create httpd event base");
        return -1;
    }
    webui_httpd = evhttp_new(webui_base);
    if (!webui_httpd)
    {
        log_error("Failed to create evhttp event");
        return -1;
    }
    int rc = pthread_create(&handle, NULL, thread_main, context);
    if (!rc)
        pthread_detach(handle);
    return rc;
}

void
stop_web_ui(void)
{
    log_debug("Stopping Web UI");
    if (webui_listener && webui_httpd)
        evhttp_del_accept_socket(webui_httpd, webui_listener);
    if (webui_httpd)
        evhttp_free(webui_httpd);
    if (webui_base)
        event_base_loopbreak(webui_base);
}

