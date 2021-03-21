/**
 * @file
 * Test code for Config Synonyms
 *
 * @authors
 * Copyright (C) 2017-2018 Richard Russon <rich@flatcap.org>
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
#include "config.h"
#include "acutest.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "mutt/lib.h"
#include "config/common.h"
#include "config/lib.h"
#include "core/lib.h"

static char *VarApple;
static char *VarCherry;
static char *VarElderberry;
static char *VarGuava;
static char *VarIlama;

// clang-format off
static struct ConfigDef Vars[] = {
  { "Apple",      DT_STRING,  0,               0, NULL, NULL, &VarApple,      },
  { "Banana",     DT_SYNONYM, IP "Apple",      0, NULL, NULL, NULL,           },
  { "Cherry",     DT_STRING,  IP "cherry",     0, NULL, NULL, &VarCherry,     },
  { "Damson",     DT_SYNONYM, IP "Cherry",     0, NULL, NULL, NULL,           },
  { "Elderberry", DT_STRING,  0,               0, NULL, NULL, &VarElderberry, },
  { "Fig",        DT_SYNONYM, IP "Elderberry", 0, NULL, NULL, NULL,           },
  { "Guava",      DT_STRING,  0,               0, NULL, NULL, &VarGuava,      },
  { "Hawthorn",   DT_SYNONYM, IP "Guava",      0, NULL, NULL, NULL,           },
  { "Ilama",      DT_STRING,  IP "iguana",     0, NULL, NULL, &VarIlama,      },
  { "Jackfruit",  DT_SYNONYM, IP "Ilama",      0, NULL, NULL, NULL,           },
  { NULL },
};

static struct ConfigDef Vars2[] = {
  { "Jackfruit",  DT_SYNONYM, IP "Broken",     0, NULL },
  { NULL },
};
// clang-format on

static bool test_string_set(struct ConfigSet *cs, struct Buffer *err)
{
  log_line(__func__);

  const char *name = "Banana";
  const char *value = "pudding";

  mutt_buffer_reset(err);
  int rc = cs_str_string_set(cs, name, value, err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", err->data);
    return false;
  }

  if (!TEST_CHECK(mutt_str_equal(VarApple, value)))
  {
    TEST_MSG("Value of %s wasn't changed\n", name);
    return false;
  }
  TEST_MSG("%s = %s, set by '%s'\n", name, NONULL(VarApple), value);

  return true;
}

static bool test_string_get(struct ConfigSet *cs, struct Buffer *err)
{
  log_line(__func__);
  const char *name = "Damson";

  mutt_buffer_reset(err);
  int rc = cs_str_string_get(cs, name, err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("Get failed: %s\n", err->data);
    return false;
  }
  TEST_MSG("%s = '%s', '%s'\n", name, NONULL(VarCherry), err->data);

  return true;
}

static bool test_native_set(struct ConfigSet *cs, struct Buffer *err)
{
  log_line(__func__);

  const char *name = "Fig";
  const char *value = "tree";

  mutt_buffer_reset(err);
  int rc = cs_str_native_set(cs, name, (intptr_t) value, err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", err->data);
    return false;
  }

  if (!TEST_CHECK(mutt_str_equal(VarElderberry, value)))
  {
    TEST_MSG("Value of %s wasn't changed\n", name);
    return false;
  }
  TEST_MSG("%s = %s, set by '%s'\n", name, NONULL(VarElderberry), value);

  return true;
}

static bool test_native_get(struct ConfigSet *cs, struct Buffer *err)
{
  log_line(__func__);
  const char *name = "Hawthorn";

  int rc = cs_str_string_set(cs, name, "tree", err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
    return false;

  mutt_buffer_reset(err);
  intptr_t value = cs_str_native_get(cs, name, err);
  if (!TEST_CHECK(mutt_str_equal(VarGuava, (const char *) value)))
  {
    TEST_MSG("Get failed: %s\n", err->data);
    return false;
  }
  TEST_MSG("%s = '%s', '%s'\n", name, VarGuava, (const char *) value);

  return true;
}

static bool test_reset(struct ConfigSet *cs, struct Buffer *err)
{
  log_line(__func__);

  const char *name = "Jackfruit";
  mutt_buffer_reset(err);

  TEST_MSG("Initial: %s = '%s'\n", name, NONULL(VarIlama));
  int rc = cs_str_string_set(cs, name, "hello", err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
    return false;
  TEST_MSG("Set: %s = '%s'\n", name, VarIlama);

  mutt_buffer_reset(err);
  rc = cs_str_reset(cs, name, err);
  if (!TEST_CHECK(CSR_RESULT(rc) == CSR_SUCCESS))
  {
    TEST_MSG("%s\n", err->data);
    return false;
  }

  if (!TEST_CHECK(mutt_str_equal(VarIlama, "iguana")))
  {
    TEST_MSG("Value of %s wasn't changed\n", name);
    return false;
  }

  TEST_MSG("Reset: %s = '%s'\n", name, VarIlama);

  return true;
}

void test_config_synonym(void)
{
  log_line(__func__);

  struct Buffer err;
  mutt_buffer_init(&err);
  err.dsize = 256;
  err.data = mutt_mem_calloc(1, err.dsize);
  mutt_buffer_reset(&err);

  struct ConfigSet *cs = cs_new(30);
  NeoMutt = neomutt_new(cs);

  cs_register_type(cs, &cst_string);
  if (!cs_register_variables(cs, Vars, 0))
    return;

  if (cs_register_variables(cs, Vars2, 0))
  {
    TEST_MSG("Test should have failed\n");
    return;
  }

  TEST_MSG("Expected error\n");

  notify_observer_add(NeoMutt->notify, NT_CONFIG, log_observer, 0);

  set_list(cs);

  TEST_CHECK(test_string_set(cs, &err));
  TEST_CHECK(test_string_get(cs, &err));
  TEST_CHECK(test_native_set(cs, &err));
  TEST_CHECK(test_native_get(cs, &err));
  TEST_CHECK(test_reset(cs, &err));

  neomutt_free(&NeoMutt);
  cs_free(&cs);
  FREE(&err.data);
}
