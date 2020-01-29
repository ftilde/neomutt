/**
 * @file
 * Shared constants/structs that are private to Mbox
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

#ifndef MUTT_MBOX_MBOX_PRIVATE_H
#define MUTT_MBOX_MBOX_PRIVATE_H

#include "core/lib.h"

struct MuttPath;
struct stat;

int              mbox_path2_canon  (struct MuttPath *path);
int              mbox_path2_compare(struct MuttPath *path1, struct MuttPath *path2);
int              mbox_path2_parent (const struct MuttPath *path, struct MuttPath **parent);
int              mbox_path2_pretty (const struct MuttPath *path, const char *folder, char **pretty);
enum MailboxType mbox_path2_probe  (struct MuttPath *path, const struct stat *st);
int              mbox_path2_tidy   (struct MuttPath *path);

#endif /* MUTT_MBOX_MBOX_PRIVATE_H */
