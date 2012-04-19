/* Copyright 2010, 2011 NORDUnet A/S. All rights reserved.
   See the file COPYING for licensing information.  */

#if defined HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <radsec/radsec.h>
#include <radsec/radsec-impl.h>

static const char *_errtxt[] = {
  "SUCCESS",			/* 0 RSE_OK */
  "out of memory",		/* 1 RSE_NOMEM */
  "not yet implemented",	/* 2 RSE_NOSYS */
  "invalid handle",		/* 3 RSE_INVALID_CTX */
  "invalid connection",		/* 4 RSE_INVALID_CONN */
  "connection type mismatch",	/* 5 RSE_CONN_TYPE_MISMATCH */
  "FreeRadius error",		/* 6 RSE_FR */
  "bad hostname or port",	/* 7 RSE_BADADDR */
  "no peer configured",		/* 8 RSE_NOPEER */
  "libevent error",		/* 9 RSE_EVENT */
  "socket error",		/* 10 RSE_SOCKERR */
  "invalid configuration file",	/* 11 RSE_CONFIG */
  "authentication failed",	/* 12 RSE_BADAUTH */
  "internal error",		/* 13 RSE_INTERNAL */
  "SSL error",			/* 14 RSE_SSLERR */
  "invalid packet",		/* 15 RSE_INVALID_PKT */
  "connect timeout",		/* 16 RSE_TIMEOUT_CONN */
  "invalid argument",		/* 17 RSE_INVAL */
  "I/O timeout",		/* 18 RSE_TIMEOUT_IO */
  "timeout",			/* 19 RSE_TIMEOUT */
  "peer disconnected",		/* 20 RSE_DISCO */
};
#define ERRTXT_SIZE (sizeof(_errtxt) / sizeof(*_errtxt))

static struct rs_error *
_err_vcreate (unsigned int code, const char *file, int line, const char *fmt,
	      va_list args)
{
  struct rs_error *err = NULL;

  err = malloc (sizeof(struct rs_error));
  if (err)
    {
      int n;
      memset (err, 0, sizeof(struct rs_error));
      err->code = code;
      if (fmt)
	n = vsnprintf (err->buf, sizeof(err->buf), fmt, args);
      else
	{
	  strncpy (err->buf,
		   err->code < ERRTXT_SIZE ? _errtxt[err->code] : "",
		   sizeof(err->buf));
	  n = strlen (err->buf);
	}
      if (n >= 0 && file)
	{
	  char *sep = strrchr (file, '/');
	  if (sep)
	    file = sep + 1;
	  snprintf (err->buf + n, sizeof(err->buf) - n, " (%s:%d)", file,
		    line);
	}
    }
  return err;
}

struct rs_error *
err_create (unsigned int code,
	    const char *file,
	    int line,
	    const char *fmt,
	    ...)
{
  struct rs_error *err = NULL;

  va_list args;
  va_start (args, fmt);
  err = _err_vcreate (code, file, line, fmt, args);
  va_end (args);

  return err;
}

static int
_ctx_err_vpush_fl (struct rs_context *ctx, int code, const char *file,
		   int line, const char *fmt, va_list args)
{
  struct rs_error *err = _err_vcreate (code, file, line, fmt, args);

  if (!err)
    return RSE_NOMEM;

  /* TODO: Implement a stack.  */
  if (ctx->err)
    rs_err_free (ctx->err);
  ctx->err = err;

  return err->code;
}

int
rs_err_ctx_push (struct rs_context *ctx, int code, const char *fmt, ...)
{
  int r = 0;
  va_list args;

  va_start (args, fmt);
  r = _ctx_err_vpush_fl (ctx, code, NULL, 0, fmt, args);
  va_end (args);

  return r;
}

int
rs_err_ctx_push_fl (struct rs_context *ctx, int code, const char *file,
		    int line, const char *fmt, ...)
{
  int r = 0;
  va_list args;

  va_start (args, fmt);
  r = _ctx_err_vpush_fl (ctx, code, file, line, fmt, args);
  va_end (args);

  return r;
}

int
err_conn_push_err (struct rs_connection *conn, struct rs_error *err)
{

  if (conn->err)
    rs_err_free (conn->err);
  conn->err = err;		/* FIXME: use a stack */

  return err->code;
}

static int
_conn_err_vpush_fl (struct rs_connection *conn, int code, const char *file,
		    int line, const char *fmt, va_list args)
{
  struct rs_error *err = _err_vcreate (code, file, line, fmt, args);

  if (!err)
    return RSE_NOMEM;

  return err_conn_push_err (conn, err);
}

int
rs_err_conn_push (struct rs_connection *conn, int code, const char *fmt, ...)
{
  int r = 0;

  va_list args;
  va_start (args, fmt);
  r = _conn_err_vpush_fl (conn, code, NULL, 0, fmt, args);
  va_end (args);

  return r;
}

int
rs_err_conn_push_fl (struct rs_connection *conn, int code, const char *file,
		     int line, const char *fmt, ...)
{
  int r = 0;

  va_list args;
  va_start (args, fmt);
  r = _conn_err_vpush_fl (conn, code, file, line, fmt, args);
  va_end (args);

  return r;
}

struct rs_error *
rs_err_ctx_pop (struct rs_context *ctx)
{
  struct rs_error *err;

  if (!ctx)
    return NULL;		/* FIXME: RSE_INVALID_CTX.  */
  err = ctx->err;
  ctx->err = NULL;

  return err;
}

struct rs_error *
rs_err_conn_pop (struct rs_connection *conn)
{
  struct rs_error *err;

  if (!conn)
    return NULL;		/* FIXME: RSE_INVALID_CONN */
  err = conn->err;
  conn->err = NULL;

  return err;
}

int
rs_err_conn_peek_code (struct rs_connection *conn)
{
  if (!conn)
    return -1;			/* FIXME: RSE_INVALID_CONN */
  if (conn->err)
    return conn->err->code;

  return RSE_OK;
}

void
rs_err_free (struct rs_error *err)
{
  assert (err);
  free (err);
}

char *
rs_err_msg (struct rs_error *err)
{
  if (!err)
    return NULL;

  return err->buf;
}

int
rs_err_code (struct rs_error *err, int dofree_flag)
{
  int code;

  if (!err)
    return -1;
  code = err->code;

  if (dofree_flag)
    rs_err_free (err);

  return code;
}