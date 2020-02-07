/**
 * @file
 * Mailbox path
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
 * @page core_path Mailbox path
 *
 * Mailbox path
 */

#include "config.h"
#include "mutt/lib.h"
#include "path.h"

/**
 * mutt_path_free - Free a Path
 * @param ptr Path to free
 */
void mutt_path_free(struct MuttPath **ptr)
{
  if (!ptr || !*ptr)
    return;

  struct MuttPath *path = *ptr;

  // 'orig' may be shared as the canonical path
  if (path->canon != path->orig)
    FREE(&path->canon);
  FREE(&path->orig);
  FREE(ptr);
}

/**
 * mutt_path_new - Create a Path
 * @retval ptr New Path
 */
struct MuttPath *mutt_path_new(void)
{
  return mutt_mem_calloc(1, sizeof(struct MuttPath));
}
