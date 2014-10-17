#ifndef C_BLOCK_MYSQL_H
#define C_BLOCK_MYSQL_H
#include "uv.h"
#include <mysql.h>

struct c_mysql_query_ctx;

struct c_mysql_connection {
  uv_work_t req;
  MYSQL *mysql;
  MYSQL_RES *res;
  MYSQL_ROW row;
  int connected;
  struct c_mysql_connection *next;
};

struct c_mysql_pool {
// private:
  struct c_mysql_connection *conn_array;
  struct c_mysql_connection *free_link;
  struct c_mysql_query_ctx *first_ctx;
  struct c_mysql_query_ctx *last_ctx;
// public:
  uv_loop_t *uvloop;
  int connections;
  int max_pending_ctx;
  int max_secs;
  int pending_ctxs;
  const char *host;
  const char *user;
  const char *passwd;
  const char *db;
};

struct c_mysql_query_ctx {
  struct c_mysql_pool *mysql_pool;
  char *sql;
  int result;
  int kill_after_secs;
  struct c_mysql_connection *conn;
  struct c_mysql_query_ctx *next;
  struct c_mysql_query_ctx *pre;
};
int c_mysql_query(struct c_mysql_query_ctx *ctx);
int c_mysql_query_cleanup(struct c_mysql_query_ctx *ctx);

#endif

