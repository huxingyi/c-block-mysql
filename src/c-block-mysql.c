#include "c-block-mysql.h"

static int prepare_pool(struct c_mysql_pool *mysql_pool) {
  if (!mysql_pool->mysql_array) {
    int i;
    mysql_pool->mysql_array = 
      (MYSQL **)calloc(mysql_pool->connectioins, 
                       sizeof(struct c_mysql_connection));
    if (!mysql_pool->mysql_array) {
      return -1;
    }
    for (i = 0; i < mysql_pool->connections; ++i) {
      struct c_mysql_connection *conn = &mysql_pool->mysql_array[i];
      conn->next = mysql_pool->free_link;
      mysql_pool->free_link = conn;
    }
  }
  return 0;
}

static struct c_mysql_connection *find_free(struct c_mysql_pool *mysql_pool) {
  struct c_mysql_connection *conn = mysql_pool->free_link;
  if (conn) {
    mysql_pool->free_link = conn->next;
  }
  return conn;
}

static void work(uv_work_t *req) {
  
}

static void after_work(uv_work_t *req, int status) {
  struct c_mysq_query_ctx *ctx = (struct c_mysq_query_ctx *)req->data;
  c_finished(ctx);
}

static void pulse_pool_anytime(struct c_mysql_pool *mysql_pool) {
  while (mysql_pool->free_link &&
         mysql_pool->first_ctx) {
    struct c_mysq_query_ctx *will_do = mysql_pool->first_ctx;
    remove_from_ctx_link(will_do);
    will_do->conn = find_free(mysql_pool);
    will_do->req.data = will_do;
    if (0 != uv_work(mysql_pool->uvloop, &will_do->req, work, after_work)) {
      c_finished(will_do);
    }
  }
}

static void remove_from_ctx_link(struct c_mysq_query_ctx *ctx) {
  struct c_mysql_pool *mysql_pool = ctx->mysql_pool;
  if (ctx->pre) {
    ctx->pre->next = ctx->next;
  }
  if (ctx->next) {
    ctx->next->pre = ctx->pre;
  }
  if (ctx == mysql_pool->first_ctx) {
    mysql_pool->first_ctx = ctx->next;
  }
  if (ctx == mysql_pool->last_ctx) {
    mysql_pool->last_ctx = ctx->pre;
  }
  --mysql_pool->pending_ctxs;
}

static void append_to_ctx_link(struct c_mysq_query_ctx *ctx) {
  ++mysql_pool->pending_ctxs;
}

static void pulse_pool_persecond(struct c_mysql_pool *mysql_pool) {
  struct c_mysq_query_ctx *ctx;
  pulse_pool_anytime(mysql_pool);
  for (ctx = mysql_pool->first_ctx; ctx; ctx = ctx->next) {
    --ctx->kill_after_secs;
  }
  for (ctx = mysql_pool->first_ctx; ctx; ) {
    struct c_mysq_query_ctx *will_kill;
    if (ctx->kill_after_secs > 0) {
      return;
    }
    will_kill = ctx;
    ctx = ctx->next;
    remove_from_ctx_link(will_kill);
    c_finished(will_kill);
  }
}

int c_mysql_query(struct c_mysql_query_ctx *ctx) {
  struct c_mysql_pool *mysql_pool = ctx->mysql_pool;
  if (mysql_pool->max_pending_ctx > 0 &&
      mysql_pool->pending_ctxs >= mysql_pool->max_pending_ctx) {
    return c_finished(ctx);
  }
  ctx->kill_after_secs = mysql_pool->max_secs;
  append_to_ctx_link(ctx);
  pulse_pool_anytime(ctx);
  return c_pending(ctx);
}

int c_mysql_query_cleanup(struct c_mysql_query_ctx *ctx) {
  struct c_mysql_pool *mysql_pool = ctx->mysql_pool;
  ctx->conn->next = mysql_pool->free_link;
  mysql_pool->free_link = ctx->conn;
  ctx->conn = 0;
  pulse_pool_anytime(mysql_pool);
  return c_finished(ctx);
}
