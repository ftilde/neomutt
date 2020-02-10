/**
 * @file
 * Parse and identify different URI schemes
 *
 * @authors
 * Copyright (C) 2000-2002,2004 Thomas Roessler <roessler@does-not-exist.org>
 *
 * @copyright
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @page email_uri Parse and identify different URI schemes
 *
 * Parse and identify different URI schemes
 */

#include "config.h"
#include <ctype.h>
#include <string.h>
#include "mutt/mutt.h"
#include "uri.h"
#include "mime.h"

/**
 * UriMap - Constants for URI protocols
 */
static const struct Mapping UriMap[] = {
  { "file", U_FILE },   { "imap", U_IMAP },     { "imaps", U_IMAPS },
  { "pop", U_POP },     { "pops", U_POPS },     { "news", U_NNTP },
  { "snews", U_NNTPS }, { "mailto", U_MAILTO }, { "notmuch", U_NOTMUCH },
  { "smtp", U_SMTP },   { "smtps", U_SMTPS },   { NULL, U_UNKNOWN },
};

/**
 * parse_query_string - Parse a URI query string
 * @param list List to store the results
 * @param src  String to parse
 * @retval  0 Success
 * @retval -1 Error
 */
static int parse_query_string(struct UriQueryList *list, char *src)
{
  struct UriQuery *qs = NULL;
  char *k = NULL, *v = NULL;

  while (src && *src)
  {
    qs = mutt_mem_calloc(1, sizeof(struct UriQuery));
    k = strchr(src, '&');
    if (k)
      *k = '\0';

    v = strchr(src, '=');
    if (v)
    {
      *v = '\0';
      qs->value = v + 1;
      if (uri_pct_decode(qs->value) < 0)
      {
        FREE(&qs);
        return -1;
      }
    }
    qs->name = src;
    if (uri_pct_decode(qs->name) < 0)
    {
      FREE(&qs);
      return -1;
    }
    STAILQ_INSERT_TAIL(list, qs, entries);

    if (!k)
      break;
    src = k + 1;
  }
  return 0;
}

/**
 * uri_pct_decode - Decode a percent-encoded string
 * @param s String to decode
 * @retval  0 Success
 * @retval -1 Error
 *
 * e.g. turn "hello%20world" into "hello world"
 * The string is decoded in-place.
 */
int uri_pct_decode(char *s)
{
  if (!s)
    return -1;

  char *d = NULL;

  for (d = s; *s; s++)
  {
    if (*s == '%')
    {
      if ((s[1] != '\0') && (s[2] != '\0') && isxdigit((unsigned char) s[1]) &&
          isxdigit((unsigned char) s[2]) && (hexval(s[1]) >= 0) && (hexval(s[2]) >= 0))
      {
        *d++ = (hexval(s[1]) << 4) | (hexval(s[2]));
        s += 2;
      }
      else
        return -1;
    }
    else
      *d++ = *s;
  }
  *d = '\0';
  return 0;
}

/**
 * uri_check_scheme - Check the protocol of a URI
 * @param s String to check
 * @retval num Uri type, e.g. #U_IMAPS
 */
enum UriScheme uri_check_scheme(const char *s)
{
  char sbuf[256];
  char *t = NULL;
  int i;

  if (!s || !(t = strchr(s, ':')))
    return U_UNKNOWN;
  if ((size_t)(t - s) >= (sizeof(sbuf) - 1))
    return U_UNKNOWN;

  mutt_str_strfcpy(sbuf, s, t - s + 1);
  mutt_str_strlower(sbuf);

  i = mutt_map_get_value(sbuf, UriMap);
  if (i == -1)
    return U_UNKNOWN;

  return (enum UriScheme) i;
}

/**
 * uri_free - Free the contents of a URI
 * @param ptr Uri to free
 */
void uri_free(struct Uri **ptr)
{
  if (!ptr || !*ptr)
    return;

  struct Uri *uri = *ptr;

  struct UriQuery *np = NULL;
  struct UriQuery *tmp = NULL;
  STAILQ_FOREACH_SAFE(np, &uri->query_strings, entries, tmp)
  {
    STAILQ_REMOVE(&uri->query_strings, np, UriQuery, entries);
    // Don't free 'name', 'value': they are pointers into the 'src' string
    FREE(&np);
  }

  FREE(&uri->src);
  FREE(ptr);
}

/**
 * uri_new - Create a Uri
 * @retval ptr New Uri
 */
struct Uri *uri_new(void)
{
  struct Uri *uri = mutt_mem_calloc(1, sizeof(struct Uri));

  uri->scheme = U_UNKNOWN;
  STAILQ_INIT(&uri->query_strings);

  return uri;
}

/**
 * uri_parse - Fill in Uri
 * @param src   String to parse
 * @retval ptr  Pointer to the parsed URI
 * @retval NULL String is invalid
 *
 * To free Uri, caller must call uri_free()
 */
struct Uri *uri_parse(const char *src)
{
  if (!src || !*src)
    return NULL;

  enum UriScheme scheme = uri_check_scheme(src);
  if (scheme == U_UNKNOWN)
    return NULL;

  char *p = NULL;
  size_t srcsize = strlen(src) + 1;
  struct Uri *uri = uri_new();

  uri->scheme = scheme;
  uri->src = mutt_str_strdup(src);

  char *it = uri->src;

  it = strchr(it, ':') + 1;

  if (strncmp(it, "//", 2) != 0)
  {
    uri->path = it;
    if (uri_pct_decode(uri->path) < 0)
    {
      uri_free(&uri);
    }
    return uri;
  }

  it += 2;

  /* We have the length of the string, so let's be fancier than strrchr */
  for (char *q = uri->src + srcsize - 1; q >= it; --q)
  {
    if (*q == '?')
    {
      *q = '\0';
      if (parse_query_string(&uri->query_strings, q + 1) < 0)
      {
        goto err;
      }
      break;
    }
  }

  uri->path = strchr(it, '/');
  if (uri->path)
  {
    *uri->path++ = '\0';
    if (uri_pct_decode(uri->path) < 0)
      goto err;
  }

  char *at = strrchr(it, '@');
  if (at)
  {
    *at = '\0';
    p = strchr(it, ':');
    if (p)
    {
      *p = '\0';
      uri->pass = p + 1;
      if (uri_pct_decode(uri->pass) < 0)
        goto err;
    }
    uri->user = it;
    if (uri_pct_decode(uri->user) < 0)
      goto err;
    it = at + 1;
  }

  /* IPv6 literal address.  It may contain colons, so set p to start the port
   * scan after it.  */
  if ((*it == '[') && (p = strchr(it, ']')))
  {
    it++;
    *p++ = '\0';
  }
  else
    p = it;

  p = strchr(p, ':');
  if (p)
  {
    int num;
    *p++ = '\0';
    if ((mutt_str_atoi(p, &num) < 0) || (num < 0) || (num > 0xffff))
      goto err;
    uri->port = (unsigned short) num;
  }
  else
    uri->port = 0;

  if (mutt_str_strlen(it) != 0)
  {
    uri->host = it;
    if (uri_pct_decode(uri->host) < 0)
      goto err;
  }
  else if (uri->path)
  {
    /* No host are provided, we restore the / because this is absolute path */
    uri->path = it;
    *it++ = '/';
  }

  return uri;

err:
  uri_free(&uri);
  return NULL;
}

/**
 * uri_pct_encode - Percent-encode a string
 * @param buf    Buffer for the result
 * @param buflen Length of buffer
 * @param src String to encode
 *
 * e.g. turn "hello world" into "hello%20world"
 */
void uri_pct_encode(char *buf, size_t buflen, const char *src)
{
  static const char *hex = "0123456789ABCDEF";

  if (!buf)
    return;

  *buf = '\0';
  buflen--;
  while (src && *src && (buflen != 0))
  {
    if (strchr("/:&%=", *src))
    {
      if (buflen < 3)
        break;

      *buf++ = '%';
      *buf++ = hex[(*src >> 4) & 0xf];
      *buf++ = hex[*src & 0xf];
      src++;
      buflen -= 3;
      continue;
    }
    *buf++ = *src++;
    buflen--;
  }
  *buf = '\0';
}

/**
 * uri_tobuffer - Output the URI string for a given Uri object
 * @param uri    Uri to turn into a string
 * @param buf    Buffer for the result
 * @param flags  Flags, e.g. #U_PATH
 * @retval  0 Success
 * @retval -1 Error
 */
int uri_tobuffer(struct Uri *uri, struct Buffer *buf, int flags)
{
  if (!uri || !buf)
    return -1;
  if (uri->scheme == U_UNKNOWN)
    return -1;

  mutt_buffer_printf(buf, "%s:", mutt_map_get_name(uri->scheme, UriMap));

  if (uri->host)
  {
    if (!(flags & U_PATH))
      mutt_buffer_addstr(buf, "//");

    if (uri->user && (uri->user[0] || !(flags & U_PATH)))
    {
      char str[256];
      uri_pct_encode(str, sizeof(str), uri->user);
      mutt_buffer_add_printf(buf, "%s@", str);
    }

    if (strchr(uri->host, ':'))
      mutt_buffer_add_printf(buf, "[%s]", uri->host);
    else
      mutt_buffer_add_printf(buf, "%s", uri->host);

    if (uri->port)
      mutt_buffer_add_printf(buf, ":%hu/", uri->port);
    else
      mutt_buffer_addstr(buf, "/");
  }

  if (uri->path)
    mutt_buffer_addstr(buf, uri->path);

  if (STAILQ_FIRST(&uri->query_strings))
  {
    mutt_buffer_addstr(buf, "?");

    char str[256];
    struct UriQuery *np = NULL;
    STAILQ_FOREACH(np, &uri->query_strings, entries)
    {
      uri_pct_encode(str, sizeof(str), np->name);
      mutt_buffer_addstr(buf, str);
      mutt_buffer_addstr(buf, "=");
      uri_pct_encode(str, sizeof(str), np->value);
      mutt_buffer_addstr(buf, str);
      if (STAILQ_NEXT(np, entries))
        mutt_buffer_addstr(buf, "&");
    }
  }

  return 0;
}

/**
 * uri_tostring - Output the URI string for a given Uri object
 * @param uri    Uri to turn into a string
 * @param dest   Buffer for the result
 * @param len    Length of buffer
 * @param flags  Flags, e.g. #U_PATH
 * @retval  0 Success
 * @retval -1 Error
 */
int uri_tostring(struct Uri *uri, char *dest, size_t len, int flags)
{
  if (!uri || !dest)
    return -1;

  struct Buffer *dest_buf = mutt_buffer_pool_get();

  int retval = uri_tobuffer(uri, dest_buf, flags);
  if (retval == 0)
    mutt_str_strfcpy(dest, mutt_b2s(dest_buf), len);

  mutt_buffer_pool_release(&dest_buf);

  return retval;
}
