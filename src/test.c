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
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

struct task_ctx {
  struct c_block block;
  struct c_mysql_pool *mysql_pool;
  union {
    struct c_mysql_query_ctx mysql;
  } u;
};

static int c_task(struct task_ctx *x) {
  printf("enter c_task\n");
  c_begin(x, c_task);
  x->u.mysql.mysql_pool = x->mysql_pool;
  x->u.mysql.sql = "insert into hello_table(itemname) values('hello')";
  x->u.mysql.sqllen = strlen(x->u.mysql.sql);
  x->u.mysql.result = -1;
  c_await(&x->u.mysql, c_mysql_query);
  printf("mysql_affected_rows:%d\n", 
    mysql_affected_rows(x->u.mysql.conn->mysql));
  c_mysql_query_cleanup(&x->u.mysql);
  printf("x->u.mysql.result:%d\n", x->u.mysql.result);
  c_end();
  return c_finished(x);
}

static int on_free_task(struct task_ctx *ctx) {
  printf("free ctx\n");
  free(ctx);
  return 0;
}

static void test_mysql(struct c_mysql_pool *mysql_pool) {
  struct task_ctx *ctx = (struct task_ctx *)malloc(sizeof(struct task_ctx));
  ctx->mysql_pool = mysql_pool;
  c_spawn(ctx, c_task, on_free_task);
}

int main(int argc, char *argv[]) {

  struct c_mysql_pool mysql_pool;
  
  printf("enter main\n");
  
  mysql_library_init(0, 0, 0);
  
  mysql_pool.uvloop = uv_default_loop();
  mysql_pool.connections = 1;
  mysql_pool.max_pending_ctx = 1024;
  mysql_pool.max_secs = 6;
  mysql_pool.host = "localhost";
  mysql_pool.user = "hello_user";
  mysql_pool.passwd = "hello_password";
  mysql_pool.db = "test";
  mysql_pool.port = 0;
  c_mysql_init_pool(&mysql_pool);
  
  test_mysql(&mysql_pool);
  
  uv_run(uv_default_loop(), UV_RUN_DEFAULT);
  
  printf("leave main\n");
  
  return 0;
}
