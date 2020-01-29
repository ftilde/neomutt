/**
 * @file
 * Mailbox path functions
 *
 * @authors
 * Copyright (C) 2020 Richard Russon <rich@flatcap.org>
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
 * @page mx_path Mailbox path functions
 *
 * Mailbox path functions
 */

#include "config.h"
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include "email/lib.h"
#include "alias.h"
#include "globals.h"
#include "hook.h"
#include "mx.h"

/**
 * path2_tidy - Tidy a Mailbox path
 */
static int path2_tidy(struct MuttPath *path)
{
  return -1;
}

/**
 * path2_resolve - Resolve special strings in a Mailbox Path
 */
static int path2_resolve(struct MuttPath *path)
{
  return -1;
}

/**
 * mx_path2_canon - XXX
 */
int mx_path2_canon(struct MuttPath *path)
{
  return -1;
}

/**
 * mx_path2_compare - XXX
 */
int mx_path2_compare(struct MuttPath *path1, struct MuttPath *path2)
{
  return -1;
}

/**
 * mx_path2_parent - XXX
 * @retval -1 Error
 * @retval  0 Success, parent returned
 * @retval  1 Success, path is has no parent
 */
int mx_path2_parent(const struct MuttPath *path, struct MuttPath **parent)
{
  return -1;
}

/**
 * mx_path2_pretty - XXX
 */
int mx_path2_pretty(const struct MuttPath *path, const char *folder, char **pretty)
{
  return -1;
}

/**
 * mx_path2_probe - Determine the Mailbox type of a path
 * @param path Path to examine
 * @retval num XXX
 */
int mx_path2_probe(struct MuttPath *path)
{
  return -1;
}

/**
 * mx_path2_resolve - XXX
 */
int mx_path2_resolve(struct MuttPath *path)
{
  int rc = path2_resolve(path);
  if (rc < 0)
    return rc;

  rc = mx_path2_probe(path);
  if (rc < 0)
    return rc;

  rc = path2_tidy(path);
  if (rc < 0)
    return rc;

  return 0;
}
