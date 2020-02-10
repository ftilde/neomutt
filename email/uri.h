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

#ifndef MUTT_EMAIL_URI_H
#define MUTT_EMAIL_URI_H

#include <stddef.h>
#include "mutt/mutt.h"

/**
 * enum UriScheme - All recognised Uri types
 */
enum UriScheme
{
  U_UNKNOWN, ///< Uri wasn't recognised
  U_FILE,    ///< Uri is file://
  U_POP,     ///< Uri is pop://
  U_POPS,    ///< Uri is pops://
  U_IMAP,    ///< Uri is imap://
  U_IMAPS,   ///< Uri is imaps://
  U_NNTP,    ///< Uri is nntp://
  U_NNTPS,   ///< Uri is nntps://
  U_SMTP,    ///< Uri is smtp://
  U_SMTPS,   ///< Uri is smtps://
  U_MAILTO,  ///< Uri is mailto://
  U_NOTMUCH, ///< Uri is notmuch://
};

#define U_PATH          (1 << 1)

/**
 * struct UriQuery - Parsed Query String
 *
 * The arguments in a URI are saved in a linked list.
 */
struct UriQuery
{
  char *name;                     ///< Query name
  char *value;                    ///< Query value
  STAILQ_ENTRY(UriQuery) entries; ///< Linked list
};
STAILQ_HEAD(UriQueryList, UriQuery);

/**
 * struct Uri - A parsed URI `proto://user:password@host:port/path?a=1&b=2`
 */
struct Uri
{
  enum UriScheme scheme;             ///< Scheme, e.g. #U_SMTPS
  char *user;                        ///< Username
  char *pass;                        ///< Password
  char *host;                        ///< Host
  unsigned short port;               ///< Port
  char *path;                        ///< Path
  struct UriQueryList query_strings; ///< List of query strings
  char *src;                         ///< Raw URI string
};

enum UriScheme uri_check_scheme(const char *s);
void           uri_free        (struct Uri **ptr);
struct Uri    *uri_parse       (const char *src);
int            uri_pct_decode  (char *s);
void           uri_pct_encode  (char *buf, size_t buflen, const char *src);
int            uri_tobuffer    (struct Uri *uri, struct Buffer *dest, int flags);
int            uri_tostring    (struct Uri *uri, char *buf, size_t buflen, int flags);

#endif /* MUTT_EMAIL_URI_H */
