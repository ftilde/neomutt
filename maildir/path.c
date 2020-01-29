/**
 * @file
 * Maildir path manipulations
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
 * @page maildir_path Maildir path manipulations
 *
 * Maildir path manipulations
 */

#include "config.h"
#include "config/lib.h"
#include "email/lib.h"
#include "core/lib.h"

/**
 * maildir_path2_canon - Canonicalise a Mailbox path - Implements MxOps::path2_canon
 */
int maildir_path2_canon(struct MuttPath *path)
{
  return -1;
}

/**
 * maildir_path2_compare - Compare two Maibox paths - Implements MxOps::path2_compare
 */
int maildir_path2_compare(struct MuttPath *path1, struct MuttPath *path2)
{
  return -1;
}

/**
 * maildir_path2_parent - Find the parent of a Mailbox path - Implements MxOps::path2_parent
 */
int maildir_path2_parent(const struct MuttPath *path, struct MuttPath **parent)
{
  return -1;
}

/**
 * maildir_path2_pretty - Abbreviate a Mailbox path - Implements MxOps::path2_pretty
 */
int maildir_path2_pretty(const struct MuttPath *path, const char *folder, char **pretty)
{
  return -1;
}

/**
 * maildir_path2_probe - Does this Mailbox type recognise this path? - Implements MxOps::path2_probe
 */
int maildir_path2_probe(struct MuttPath *path, const struct stat *st)
{
  return -1;
}

/**
 * maildir_path2_tidy - Tidy a Mailbox path - Implements MxOps::path2_tidy
 */
int maildir_path2_tidy(struct MuttPath *path)
{
  return -1;
}

/**
 * mh_path2_probe - Does this Mailbox type recognise this path? - Implements MxOps::path2_probe
 */
int mh_path2_probe(struct MuttPath *path, const struct stat *st)
{
  return -1;
}
