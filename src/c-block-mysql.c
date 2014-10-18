/* Copyright (c) 2014, huxingyi@msn.com
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "c-block-mysql.h"

static void remove_from_ctx_link(struct c_mysql_query_ctx *ctx) {
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

static void append_to_ctx_link(struct c_mysql_query_ctx *ctx) {
  struct c_mysql_pool *mysql_pool = ctx->mysql_pool;
  ++mysql_pool->pending_ctxs;
  ctx->next = 0;
  ctx->pre = mysql_pool->last_ctx;
  if (mysql_pool->last_ctx) {
    mysql_pool->last_ctx->next = ctx;
  }
  mysql_pool->last_ctx = ctx;
  if (!mysql_pool->first_ctx) {
    mysql_pool->first_ctx = ctx;
  }
}

static int prepare_pool(struct c_mysql_pool *mysql_pool) {
  if (!mysql_pool->conn_array) {
    int i;
    mysql_pool->conn_array = 
      (struct c_mysql_connection *)calloc(mysql_pool->connections, 
                       sizeof(struct c_mysql_connection));
    if (!mysql_pool->conn_array) {
      return -1;
    }
    for (i = 0; i < mysql_pool->connections; ++i) {
      struct c_mysql_connection *conn = &mysql_pool->conn_array[i];
      conn->pool = mysql_pool;
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
  struct c_mysql_query_ctx *ctx = (struct c_mysql_query_ctx *)req->data;
  struct c_mysql_connection *conn = ctx->conn;
  struct c_mysql_pool *mysql_pool = conn->pool;
  
  if (!conn->connected) {
    char value = 1;
    if (!conn->mysql) {
      conn->mysql = mysql_init(0);
    }
    if (!conn->mysql) {
      return;
    }
    if (!mysql_real_connect(conn->mysql, mysql_pool->host, mysql_pool->user, 
                            mysql_pool->passwd, mysql_pool->db, 
                            mysql_pool->port, 0, 0)) {
      return;
    }
    mysql_options(conn->mysql, MYSQL_OPT_RECONNECT, (char *)&value);
    conn->connected = 1;
  }
  
  if (conn->res) {
    mysql_free_result(conn->res);
    conn->res = 0;
  }
  
  if (0 != mysql_real_query(conn->mysql, ctx->sql, ctx->sqllen)) {
    return;
  }
  
  conn->res = mysql_store_result(conn->mysql);
  if (!conn->res) {
    if (0 != mysql_field_count(conn->mysql)) {
      return;
    }
  }
  
  ctx->result = 0;
}

static void after_work(uv_work_t *req, int status) {
  struct c_mysql_query_ctx *ctx = (struct c_mysql_query_ctx *)req->data;
  c_finished(ctx);
}

static void pulse_pool_anytime(struct c_mysql_pool *mysql_pool) {
  while (mysql_pool->free_link &&
         mysql_pool->first_ctx) {
    struct c_mysql_query_ctx *will_do = mysql_pool->first_ctx;
    remove_from_ctx_link(will_do);
    will_do->conn = find_free(mysql_pool);
    will_do->req.data = will_do;
    will_do->result = -1;
    if (0 != uv_queue_work(mysql_pool->uvloop, &will_do->req, work, after_work)) {
      c_finished(will_do);
    }
  }
}

static void pulse_pool_persecond(struct c_mysql_pool *mysql_pool) {
  struct c_mysql_query_ctx *ctx;
  pulse_pool_anytime(mysql_pool);
  for (ctx = mysql_pool->first_ctx; ctx; ctx = ctx->next) {
    --ctx->kill_after_secs;
  }
  for (ctx = mysql_pool->first_ctx; ctx; ) {
    struct c_mysql_query_ctx *will_kill;
    if (ctx->kill_after_secs > 0) {
      return;
    }
    will_kill = ctx;
    ctx = ctx->next;
    remove_from_ctx_link(will_kill);
    c_finished(will_kill);
  }
}

static void on_per_second(uv_timer_t *req) {
  struct c_mysql_pool *mysql_pool = (struct c_mysql_pool *)req->data;
  pulse_pool_persecond(mysql_pool);
}

int c_mysql_init_pool(struct c_mysql_pool *mysql_pool) {
  mysql_pool->conn_array = 0;
  mysql_pool->free_link = 0;
  mysql_pool->first_ctx = 0;
  mysql_pool->last_ctx = 0;
  mysql_pool->max_pending_ctx = 0;
  uv_timer_init(mysql_pool->uvloop, &mysql_pool->timer_req);
  mysql_pool->timer_req.data = mysql_pool;
  uv_timer_start(&mysql_pool->timer_req, on_per_second, 1000, 1000);
  return prepare_pool(mysql_pool);
}

int c_mysql_query(struct c_mysql_query_ctx *ctx) {
  struct c_mysql_pool *mysql_pool = ctx->mysql_pool;
  if (mysql_pool->max_pending_ctx > 0 &&
      mysql_pool->pending_ctxs >= mysql_pool->max_pending_ctx) {
    return c_finished(ctx);
  }
  ctx->kill_after_secs = mysql_pool->max_secs;
  append_to_ctx_link(ctx);
  pulse_pool_anytime(mysql_pool);
  return c_pending(ctx);
}

int c_mysql_query_cleanup(struct c_mysql_query_ctx *ctx) {
  struct c_mysql_pool *mysql_pool = ctx->mysql_pool;
  ctx->conn->next = mysql_pool->free_link;
  mysql_pool->free_link = ctx->conn;
  ctx->conn = 0;
  pulse_pool_anytime(mysql_pool);
  return 0;
}
