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

#ifndef C_BLOCK_MYSQL_H
#define C_BLOCK_MYSQL_H

#include "c-block.h"
#include "uv.h"
#include <mysql.h>

struct c_mysql_pool;
struct c_mysql_query_ctx;

struct c_mysql_connection {
  uv_work_t req;
  MYSQL *mysql;
  MYSQL_RES *res;
  int connected;
  struct c_mysql_pool *pool;
  struct c_mysql_connection *next;
};

struct c_mysql_pool {
// private:
  struct c_mysql_connection *conn_array;
  struct c_mysql_connection *free_link;
  struct c_mysql_query_ctx *first_ctx;
  struct c_mysql_query_ctx *last_ctx;
  int max_pending_ctx;
  uv_timer_t timer_req;
// public:
  uv_loop_t *uvloop;
  int connections;
  int max_secs;
  int pending_ctxs;
  const char *host;
  const char *user;
  const char *passwd;
  const char *db;
  unsigned int port;
};
int c_mysql_init_pool(struct c_mysql_pool *mysql_pool);

struct c_mysql_query_ctx {
// private:
  uv_work_t req;
  struct c_block block;
  int kill_after_secs;
  struct c_mysql_connection *conn;
  struct c_mysql_query_ctx *next;
  struct c_mysql_query_ctx *pre;
// public:
  struct c_mysql_pool *mysql_pool;
  char *sql;
  int sqllen;
  int result;
};
c_async int c_mysql_query(struct c_mysql_query_ctx *ctx);
int c_mysql_query_cleanup(struct c_mysql_query_ctx *ctx);

#endif

