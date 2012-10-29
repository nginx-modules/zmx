#include <ngx_config.h>
#include <ngx_core.h>

static ngx_command_t  ngx_zmx_commands[] = {

    { ngx_string("zmx_proxy"),
      NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
      ngx_zmx_proxy,
      ngx_http_loc_conf_offset,
      0,
      null },

ngx_module_t  ngx_http_zmx = {
    NGX_MODULE_V1,
    &ngx_zmx_module_ctx,                   /* module context */
    ngx_zmx_commands,                      /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_http_module_t  ngx_zmx_module_ctx = {
    NULL,                                  /* preconfiguration */
    NULL,                                  /* postconfiguration */

    NULL,                                  /* create main configuration */
    NULL,                                  /* init main configuration */

    NULL,                                  /* create server configuration */
    NULL,                                  /* merge server configuration */

    NULL,                                  /* create location configuration */
    NULL                                   /* merge location configuration */
};

typedef struct {
    ngx_http_status_t              status;
    /* ngx_http_proxy_vars_t          vars; */
    size_t                         internal_body_length;

    ngx_uint_t                     state;
    off_t                          size;
    off_t                          length;

    ngx_uint_t                     head;  /* unsigned  head:1 */
} ngx_zmx_ctx_t;

static char *
ngx_zmx_proxy(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_http_core_loc_conf_t   *clcf;

    clcf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

    clcf->handler = ngx_zmx_handler;

    return NGX_CONF_OK;
}



static ngx_int_t
ngx_zmx_handler(ngx_http_request_t *r)
{
    ngx_int_t                   rc;
    ngx_http_upstream_t        *u;
    ngx_zmx_ctx_t       *ctx;
    ngx_http_proxy_loc_conf_t  *plcf;

    if (ngx_http_upstream_create(r) != NGX_OK) {
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }

    ctx = ngx_pcalloc(r->pool, sizeof(ngx_zmx_ctx_t));
    if (ctx == NULL) {
        return NGX_ERROR;
    }

    ngx_http_set_ctx(r, ctx, ngx_zmx_module);

    /* plcf = ngx_http_get_module_loc_conf(r, ngx_zmx_module); */

    u = r->upstream;

    u->output.tag = (ngx_buf_tag_t) &ngx_zmx_module;

    u->conf = &plcf->upstream;

    u->create_request = ngx_http_proxy_create_request;
    u->reinit_request = ngx_http_proxy_reinit_request;
    u->process_header = ngx_http_proxy_process_status_line;
    u->abort_request = ngx_http_proxy_abort_request;
    u->finalize_request = ngx_http_proxy_finalize_request;
    r->state = 0;

    u->buffering = plcf->upstream.buffering;

    /* u->pipe = ngx_pcalloc(r->pool, sizeof(ngx_event_pipe_t)); */
    /* if (u->pipe == NULL) { */
    /*     return NGX_HTTP_INTERNAL_SERVER_ERROR; */
    /* } */

    /* u->accel = 1; */

    rc = ngx_http_read_client_request_body(r, ngx_http_upstream_init);

    return NGX_DONE;
}

