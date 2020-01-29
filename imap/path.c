/**
 * @file
 * IMAP path manipulations
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
 * @page imap_path IMAP path manipulations
 *
 * IMAP path manipulations
 */

#include "config.h"
#include "config/lib.h"
#include "email/lib.h"
#include "core/lib.h"

/**
 * imap_path2_canon - Canonicalise a Mailbox path - Implements MxOps::path2_canon
 * @param path   Path to canonicalise
 * @param delims Path delimiters
 * @retval  0 Success
 * @retval -1 Failure
 */
int imap_path2_canon(struct MuttPath *path, const char *delims)
{
  return -1;
}

/**
 * imap_path2_compare - Compare two Maibox paths - Implements MxOps::path2_compare
 */
int imap_path2_compare(struct MuttPath *path1, struct MuttPath *path2)
{
  return -1;
}

/**
 * imap_path2_parent - Find the parent of a Mailbox path - Implements MxOps::path2_parent
 * @param[in] path    Mailbox path
 * @param[in] delims  Path delimiters
 * @param[out] parent Parent of path
 * @retval -1 Error
 * @retval  0 Success, parent returned
 * @retval  1 Success, path is root, it has no parent
 */
int imap_path2_parent(const struct MuttPath *path, const char *delims, struct MuttPath **parent)
{
  return -1;
}

/**
 * imap_path2_pretty - Abbreviate a Mailbox path - Implements MxOps::path2_pretty
 */
int imap_path2_pretty(const struct MuttPath *path, const char *folder, char **pretty)
{
  return -1;
}

/**
 * imap_path2_probe - Does this Mailbox type recognise this path? - Implements MxOps::path2_probe
 */
int imap_path2_probe(struct MuttPath *path, const struct stat *st)
{
  return -1;
}

/**
 * imap_path2_tidy - Tidy a Mailbox path - Implements MxOps::path2_tidy
 */
int imap_path2_tidy(struct MuttPath *path)
{
  return -1;
}
