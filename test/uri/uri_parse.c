/**
 * @file
 * Test code for uri_parse()
 *
 * @authors
 * Copyright (C) 2019 Richard Russon <rich@flatcap.org>
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

#define TEST_NO_MAIN
#include "acutest.h"
#include "config.h"
#include "mutt/mutt.h"
#include "address/lib.h"
#include "email/lib.h"

struct UriTest
{
  const char *source;  // source URI to parse
  bool valid;          // expected validity
  struct Uri uri;      // expected resulting URI
  const char *qs_elem; // expected elements of the query string, separated
                       // and terminated by a pipe '|' character
};

// clang-format off
static struct UriTest test[] = {
  {
    "foobar foobar",
    false,
  },
  {
    "imaps://foouser:foopass@imap.example.com:456",
    true,
    {
      U_IMAPS,
      "foouser",
      "foopass",
      "imap.example.com",
      456,
      NULL
    },
    NULL
  },
  {
    "SmTp://user@example.com", /* scheme is lower-cased */
    true,
    {
      U_SMTP,
      "user",
      NULL,
      "example.com",
      0,
      NULL
    },
    NULL
  },
  {
    "pop://user@example.com@pop.example.com:234/some/where?encoding=binary",
    true,
    {
      U_POP,
      "user@example.com",
      NULL,
      "pop.example.com",
      234,
      "some/where",
    },
    "encoding|binary|"
  }
};
// clang-format on

void check_query_string(const char *exp, const struct UriQueryList *act)
{
  char *next = NULL;
  char tmp[64] = { 0 };
  const struct UriQuery *np = STAILQ_FIRST(act);
  while (exp && *exp)
  {
    next = strchr(exp, '|');
    mutt_str_strfcpy(tmp, exp, next - exp + 1);
    exp = next + 1;
    if (!TEST_CHECK(strcmp(tmp, np->name) == 0))
    {
      TEST_MSG("Expected: <%s>", tmp);
      TEST_MSG("Actual  : <%s>", np->name);
    }

    next = strchr(exp, '|');
    mutt_str_strfcpy(tmp, exp, next - exp + 1);
    exp = next + 1;
    if (!TEST_CHECK(strcmp(tmp, np->value) == 0))
    {
      TEST_MSG("Expected: <%s>", tmp);
      TEST_MSG("Actual  : <%s>", np->value);
    }

    np = STAILQ_NEXT(np, entries);
  }

  if (!TEST_CHECK(np == NULL))
  {
    TEST_MSG("Expected: NULL");
    TEST_MSG("Actual  : (%s, %s)", np->name, np->value);
  }
}

void test_uri_parse(void)
{
  // struct Uri *uri_parse(const char *src);

  {
    TEST_CHECK(!uri_parse(NULL));
  }

  {
    for (size_t i = 0; i < mutt_array_size(test); ++i)
    {
      struct Uri *uri = uri_parse(test[i].source);
      if (!TEST_CHECK(!((!!uri) ^ (!!test[i].valid))))
      {
        TEST_MSG("Expected: %sNULL", test[i].valid ? "not " : "");
        TEST_MSG("Actual  : %sNULL", uri ? "not " : "");
      }

      if (!uri)
        continue;

      if (!TEST_CHECK(test[i].uri.scheme == uri->scheme))
      {
        TEST_MSG("Expected: %d", test[i].uri.scheme);
        TEST_MSG("Actual  : %d", uri->scheme);
      }
      if (!TEST_CHECK(mutt_str_strcmp(test[i].uri.user, uri->user) == 0))
      {
        TEST_MSG("Expected: %s", test[i].uri.user);
        TEST_MSG("Actual  : %s", uri->user);
      }
      if (!TEST_CHECK(mutt_str_strcmp(test[i].uri.pass, uri->pass) == 0))
      {
        TEST_MSG("Expected: %s", test[i].uri.pass);
        TEST_MSG("Actual  : %s", uri->pass);
      }
      if (!TEST_CHECK(mutt_str_strcmp(test[i].uri.host, uri->host) == 0))
      {
        TEST_MSG("Expected: %s", test[i].uri.host);
        TEST_MSG("Actual  : %s", uri->host);
      }
      if (!TEST_CHECK(test[i].uri.port == uri->port))
      {
        TEST_MSG("Expected: %hu", test[i].uri.port);
        TEST_MSG("Actual  : %hu", uri->port);
      }
      if (!TEST_CHECK(mutt_str_strcmp(test[i].uri.path, uri->path) == 0))
      {
        TEST_MSG("Expected: %s", test[i].uri.path);
        TEST_MSG("Actual  : %s", uri->path);
      }
      check_query_string(test[i].qs_elem, &uri->query_strings);

      uri_free(&uri);
    }
  }
}
