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

static void fill_chart_buffers(uint64_t chart_array_len, hashrate_chart_t *chart_array, char *miner_chart_buf_x, char *miner_chart_buf_y, char *miner_chart_buf_x0, char *miner_chart_buf_y0)
{
    int cx = sprintf(miner_chart_buf_x, "[");
    int cy = sprintf(miner_chart_buf_y, "[");
    if ((cx <= 0) || (cy <=0))
    {
        memset(miner_chart_buf_x0, 0, 250000);
        memset(miner_chart_buf_y0, 0, 250000);
        sprintf(miner_chart_buf_x0, "[]");
        sprintf(miner_chart_buf_y0, "[]");
        return;
    }
    miner_chart_buf_x += cx;
    miner_chart_buf_y += cy;
    for(uint64_t i = 0; i < chart_array_len; ++i)
    {
        cx = sprintf(miner_chart_buf_x, "%"PRIu64, chart_array[i].hashrate_timestamp);
        cy = sprintf(miner_chart_buf_y, "%"PRIu64, chart_array[i].hashrate_value);
        if ((cx <= 0) || (cy <=0))
        {
            memset(miner_chart_buf_x0, 0, 250000);
            memset(miner_chart_buf_y0, 0, 250000);
            sprintf(miner_chart_buf_x0, "[]");
            sprintf(miner_chart_buf_y0, "[]");
            return;
        }
        miner_chart_buf_x += cx;
        miner_chart_buf_y += cy;
        if (i != (chart_array_len - 1))
        {
            cx = sprintf(miner_chart_buf_x, ",");
            cy = sprintf(miner_chart_buf_y, ",");
            if ((cx <= 0) || (cy <=0))
            {
                memset(miner_chart_buf_x0, 0, 250000);
                memset(miner_chart_buf_y0, 0, 250000);
                sprintf(miner_chart_buf_x0, "[]");
                sprintf(miner_chart_buf_y0, "[]");
                return;
            }
        }
        else
        {
            cx = sprintf(miner_chart_buf_x, "]");
            cy = sprintf(miner_chart_buf_y, "]");
            if ((cx <= 0) || (cy <=0))
            {
                memset(miner_chart_buf_x0, 0, 250000);
                memset(miner_chart_buf_y0, 0, 250000);
                sprintf(miner_chart_buf_x0, "[]");
                sprintf(miner_chart_buf_y0, "[]");
                return;
            }
        }
        miner_chart_buf_x += cx;
        miner_chart_buf_y += cy;
    }
}

static void fill_chart_buffers3(uint64_t chart_array_len, total_payout_chart_t *chart_array, char *miner_chart_buf_x, char *miner_chart_buf_y, char *miner_chart_buf_z,
    char *miner_chart_buf_x0, char *miner_chart_buf_y0, char *miner_chart_buf_z0)
{
    int cx = sprintf(miner_chart_buf_x, "[");
    int cy = sprintf(miner_chart_buf_y, "[");
    int cz = sprintf(miner_chart_buf_z, "[");
    if ((cx <= 0) || (cy <=0) || (cz <=0))
    {
        memset(miner_chart_buf_x0, 0, 250000);
        memset(miner_chart_buf_y0, 0, 250000);
        memset(miner_chart_buf_z0, 0, 250000);
        sprintf(miner_chart_buf_x0, "[]");
        sprintf(miner_chart_buf_y0, "[]");
        sprintf(miner_chart_buf_z0, "[]");
        return;
    }
    miner_chart_buf_x += cx;
    miner_chart_buf_y += cy;
    miner_chart_buf_z += cz;
    for(uint64_t i = 0; i < chart_array_len; ++i)
    {
        cx = sprintf(miner_chart_buf_x, "%"PRIu64, chart_array[i].total_payout_timestamp);
        cy = sprintf(miner_chart_buf_y, "%"PRIu64, chart_array[i].total_payout_value);
        cz = sprintf(miner_chart_buf_z, "%"PRIu64, chart_array[i].total_payout_count);
        if ((cx <= 0) || (cy <=0) || (cz <=0))
        {
            memset(miner_chart_buf_x0, 0, 250000);
            memset(miner_chart_buf_y0, 0, 250000);
            memset(miner_chart_buf_z0, 0, 250000);
            sprintf(miner_chart_buf_x0, "[]");
            sprintf(miner_chart_buf_y0, "[]");
            sprintf(miner_chart_buf_z0, "[]");
            return;
        }
        miner_chart_buf_x += cx;
        miner_chart_buf_y += cy;
        miner_chart_buf_z += cz;
        if (i != (chart_array_len - 1))
        {
            cx = sprintf(miner_chart_buf_x, ",");
            cy = sprintf(miner_chart_buf_y, ",");
            cz = sprintf(miner_chart_buf_z, ",");
            if ((cx <= 0) || (cy <=0) || (cz <=0))
            {
                memset(miner_chart_buf_x0, 0, 250000);
                memset(miner_chart_buf_y0, 0, 250000);
                memset(miner_chart_buf_z0, 0, 250000);
                sprintf(miner_chart_buf_x0, "[]");
                sprintf(miner_chart_buf_y0, "[]");
                sprintf(miner_chart_buf_z0, "[]");
                return;
            }
        }
        else
        {
            cx = sprintf(miner_chart_buf_x, "]");
            cy = sprintf(miner_chart_buf_y, "]");
            cz = sprintf(miner_chart_buf_z, "]");
            if ((cx <= 0) || (cy <=0) || (cz <=0))
            {
                memset(miner_chart_buf_x0, 0, 250000);
                memset(miner_chart_buf_y0, 0, 250000);
                memset(miner_chart_buf_z0, 0, 250000);
                sprintf(miner_chart_buf_x0, "[]");
                sprintf(miner_chart_buf_y0, "[]");
                sprintf(miner_chart_buf_z0, "[]");
                return;
            }
        }
        miner_chart_buf_x += cx;
        miner_chart_buf_y += cy;
        miner_chart_buf_z += cz;
    }
}

static void
send_json_stats(struct evhttp_request *req, void *arg)
{
    struct evbuffer *buf = evhttp_request_get_output_buffer(req);
    wui_context_t *context = (wui_context_t*) arg;
    struct evkeyvalq *hdrs_out = NULL;
    uint64_t ph = context->pool_stats->pool_hashrate;
    uint64_t nh = context->pool_stats->network_hashrate;
    uint64_t nd = context->pool_stats->network_difficulty;
    uint64_t height = context->pool_stats->network_height;
    uint64_t ltf = context->pool_stats->last_template_fetched;
    uint64_t lbf = context->pool_stats->last_block_found;
    uint64_t lbfh = context->pool_stats->last_block_found_height;
    uint32_t pbf = context->pool_stats->pool_blocks_found;
#ifdef P2POOL
    uint64_t p2h = 0;
    uint64_t p2h_hashrate_15m = 0;
    uint64_t p2p_incoming = 0;
    uint64_t p2p_outcoming = 0;

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
            json_object_put(root);
        }
        free(str_json_pool);
        str_json_pool = NULL;
    }

    str_json_pool = load_file_as_c_string("/home/p2pool/stats/local/stratum");

    if (str_json_pool)
    {
        json_object *root = json_tokener_parse(str_json_pool);
        if (root)
        {

            json_object *hashrate_15m = NULL;
            json_object_object_get_ex(root, "hashrate_15m", &hashrate_15m);
            if (hashrate_15m)
            {
                if (json_object_is_type(hashrate_15m, json_type_int))
                {
                    p2h_hashrate_15m = (uint64_t)json_object_get_int64(hashrate_15m);
                }
            }

            json_object_put(root);
        }
        free(str_json_pool);
        str_json_pool = NULL;
    }

    str_json_pool = load_file_as_c_string("/home/p2pool/stats/local/p2p");

    if (str_json_pool)
    {
        json_object *root = json_tokener_parse(str_json_pool);
        if (root)
        {

            json_object *connections = NULL;
            json_object_object_get_ex(root, "connections", &connections);
            if (connections)
            {
                if (json_object_is_type(connections, json_type_int))
                {
                    p2p_outcoming = (uint64_t)json_object_get_int64(connections);
                }
            }

            json_object *incoming_connections = NULL;
            json_object_object_get_ex(root, "incoming_connections", &incoming_connections);
            if (incoming_connections)
            {
                if (json_object_is_type(incoming_connections, json_type_int))
                {
                    p2p_incoming = (uint64_t)json_object_get_int64(incoming_connections);
                }
            }

            json_object_put(root);
        }
        free(str_json_pool);
        str_json_pool = NULL;
    }
#endif
    uint64_t rh = context->pool_stats->round_hashes;
    unsigned ss = context->allow_self_select;
    double mh[6] = {0};
    double mb = 0.0;
    uint64_t wc = 0;
    const char *wa = fetch_wa_cookie(req);
    const char *wa0 = "";
    double mlpval = 0.0;
    uint64_t mlpts = 0;

    uint64_t mhrmean24h = 0;
    uint64_t mhrmean24h2 = 0;

    uint64_t chart_array_len = 0;
    hashrate_chart_t *chart_array = NULL;
    uint64_t chart_array_len2 = 0;
    hashrate_chart_t *chart_array2 = NULL;
    uint64_t chart_array_len3 = 0;
    payout_chart_t *chart_array3 = NULL;
    uint64_t chart_array_len4 = 0;
    total_payout_chart_t *chart_array4 = NULL;

    char miner_chart_buf_x_init[250000];
    char miner_chart_buf_y_init[250000];
    char *miner_chart_buf_x0 = miner_chart_buf_x_init;
    char *miner_chart_buf_y0 = miner_chart_buf_y_init;
    memset(miner_chart_buf_x0, 0, 250000);
    memset(miner_chart_buf_y0, 0, 250000);
    sprintf(miner_chart_buf_x0, "[]");
    sprintf(miner_chart_buf_y0, "[]");
    char *miner_chart_buf_x = miner_chart_buf_x_init;
    char *miner_chart_buf_y = miner_chart_buf_y_init;

    char miner_chart_buf_x_init2[250000];
    char miner_chart_buf_y_init2[250000];
    char *miner_chart_buf_x02 = miner_chart_buf_x_init2;
    char *miner_chart_buf_y02 = miner_chart_buf_y_init2;
    memset(miner_chart_buf_x02, 0, 250000);
    memset(miner_chart_buf_y02, 0, 250000);
    sprintf(miner_chart_buf_x02, "[]");
    sprintf(miner_chart_buf_y02, "[]");
    char *miner_chart_buf_x2 = miner_chart_buf_x_init2;
    char *miner_chart_buf_y2 = miner_chart_buf_y_init2;

    char miner_chart_buf_x_init3[250000];
    char miner_chart_buf_y_init3[250000];
    char *miner_chart_buf_x03 = miner_chart_buf_x_init3;
    char *miner_chart_buf_y03 = miner_chart_buf_y_init3;
    memset(miner_chart_buf_x03, 0, 250000);
    memset(miner_chart_buf_y03, 0, 250000);
    sprintf(miner_chart_buf_x03, "[]");
    sprintf(miner_chart_buf_y03, "[]");
    char *miner_chart_buf_x3 = miner_chart_buf_x_init3;
    char *miner_chart_buf_y3 = miner_chart_buf_y_init3;

    char miner_chart_buf_x_init4[250000];
    char miner_chart_buf_y_init4[250000];
    char miner_chart_buf_z_init4[250000];
    char *miner_chart_buf_x04 = miner_chart_buf_x_init4;
    char *miner_chart_buf_y04 = miner_chart_buf_y_init4;
    char *miner_chart_buf_z04 = miner_chart_buf_z_init4;
    memset(miner_chart_buf_x04, 0, 250000);
    memset(miner_chart_buf_y04, 0, 250000);
    memset(miner_chart_buf_z04, 0, 250000);
    sprintf(miner_chart_buf_x04, "[]");
    sprintf(miner_chart_buf_y04, "[]");
    sprintf(miner_chart_buf_z04, "[]");
    char *miner_chart_buf_x4 = miner_chart_buf_x_init4;
    char *miner_chart_buf_y4 = miner_chart_buf_y_init4;
    char *miner_chart_buf_z4 = miner_chart_buf_z_init4;

    if (wa)
    {
        account_hr(mh, wa);
        wc = worker_count(wa);
        uint64_t balance = account_balance(wa);
        mb = (double) balance / 1E12;
    }
    else
    {
        wa = wa0;
    }

    uint64_t hrtmp = 0;
    uint64_t hrtmp2 = 0;
    uint64_t payamount = 0;

    uint64_t totamnt = 0;
    uint64_t totts = 0;
    uint64_t totcnt = 0;

    double totlpval = 0.0;

    int rc = get_24h_meanstddev_hr(wa, &hrtmp, NULL, &chart_array_len, &chart_array, &hrtmp2, NULL, &chart_array_len2, &chart_array2,
        &payamount, &mlpts, &chart_array_len3, &chart_array3, &totamnt, &totts, &totcnt, &chart_array_len4, &chart_array4);

    if ((rc == 0) && (chart_array_len > 0))
    {
        mhrmean24h = hrtmp;

        fill_chart_buffers(chart_array_len, chart_array, miner_chart_buf_x, miner_chart_buf_y, miner_chart_buf_x0, miner_chart_buf_y0);

        free(chart_array);
        chart_array = NULL;
    }
    if ((rc == 0) && (chart_array_len2 > 0))
    {
        mhrmean24h2 = hrtmp2;

        fill_chart_buffers(chart_array_len2, chart_array2, miner_chart_buf_x2, miner_chart_buf_y2, miner_chart_buf_x02, miner_chart_buf_y02);

        free(chart_array2);
        chart_array2 = NULL;
    }
    if ((rc == 0) && (chart_array_len3 > 0))
    {
        mlpval = (double) payamount / 1E12;

        fill_chart_buffers(chart_array_len3, (hashrate_chart_t *)chart_array3, miner_chart_buf_x3, miner_chart_buf_y3, miner_chart_buf_x03, miner_chart_buf_y03);

        free(chart_array3);
        chart_array3 = NULL;
    }
    if ((rc == 0) && (chart_array_len4 > 0))
    {
        totlpval = (double) totamnt / 1E12;

        fill_chart_buffers3(chart_array_len4, (total_payout_chart_t *)chart_array4, miner_chart_buf_x4, miner_chart_buf_y4, miner_chart_buf_z4,
            miner_chart_buf_x04, miner_chart_buf_y04, miner_chart_buf_z04);

        free(chart_array4);
        chart_array4 = NULL;
    }

//JSON_PRINT:
    evbuffer_add_printf(buf, "{"
            "\"pool_hashrate\":%"PRIu64","
#ifdef P2POOL
            "\"p2pool_hashrate\":%"PRIu64","
            "\"p2pool_hashrate_local\":%"PRIu64","
            "\"p2pool_incoming_connections\":%"PRIu64","
            "\"p2pool_outcoming_connections\":%"PRIu64","
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
            "\"miner_balance\":%.12f,"
            "\"miner_last_payout\":%.12f,"
            "\"miner_last_payout_ts\":%"PRIu64","
            "\"total_last_payout\":%.12f,"
            "\"total_last_payout_ts\":%"PRIu64","
            "\"total_last_payout_count\":%"PRIu64","
            "\"miner_hashrate_mean24h\":%"PRIu64","
            "\"miner_chart_x\":%s,"
            "\"miner_chart_y\":%s,"
            "\"p2pool_hashrate_mean24h\":%"PRIu64","
            "\"p2pool_chart_x\":%s,"
            "\"p2pool_chart_y\":%s,"
            "\"payout_chart_x\":%s,"
            "\"payout_chart_y\":%s,"
            "\"total_payout_chart_x\":%s,"
            "\"total_payout_chart_y\":%s,"
            "\"total_payout_chart_z\":%s,"
            "\"worker_count\": %"PRIu64
            "}",
            ph,
#ifdef P2POOL
            p2h,
            p2h_hashrate_15m,
            p2p_incoming,
            p2p_outcoming,
#endif
            rh, nh, nd, height, ltf, lbf, lbfh, pbf,
            context->payment_threshold, context->pool_fee,
            context->pool_port, context->pool_ssl_port,
            ss, context->pool_stats->connected_accounts,
            (uint64_t)mh[0],
            (uint64_t)mh[0], (uint64_t)mh[1], (uint64_t)mh[2],
            (uint64_t)mh[3], (uint64_t)mh[4], (uint64_t)mh[5], mb, mlpval, mlpts, totlpval, totts, totcnt, mhrmean24h, miner_chart_buf_x0, miner_chart_buf_y0,
            mhrmean24h2, miner_chart_buf_x02, miner_chart_buf_y02, miner_chart_buf_x03, miner_chart_buf_y03,
            miner_chart_buf_x04, miner_chart_buf_y04, miner_chart_buf_z04,
            wc);
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

