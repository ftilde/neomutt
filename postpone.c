/**
 * @file
 * Save/restore and GUI list postponed emails
 *
 * @authors
 * Copyright (C) 1996-2002,2012-2013 Michael R. Elkins <me@mutt.org>
 * Copyright (C) 1999-2002,2004 Thomas Roessler <roessler@does-not-exist.org>
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
 * @page postpone Save/restore and GUI list postponed emails
 *
 * Save/restore and GUI list postponed emails
 */

#include "config.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include "mutt/mutt.h"
#include "config/lib.h"
#include "email/lib.h"
#include "conn/conn.h"
#include "mutt.h"
#include "context.h"
#include "format_flags.h"
#include "globals.h"
#include "handler.h"
#include "hdrline.h"
#include "keymap.h"
#include "mailbox.h"
#include "mutt_logging.h"
#include "mutt_menu.h"
#include "mutt_thread.h"
#include "muttlib.h"
#include "ncrypt/ncrypt.h"
#include "opcodes.h"
#include "options.h"
#include "protos.h"
#include "send.h"
#include "sendlib.h"
#include "sort.h"
#include "state.h"
#ifdef USE_IMAP
#include "imap/imap.h"
#endif

static const struct Mapping PostponeHelp[] = {
  { N_("Exit"), OP_EXIT },
  { N_("Del"), OP_DELETE },
  { N_("Undel"), OP_UNDELETE },
  { N_("Help"), OP_HELP },
  { NULL, 0 },
};

static short PostCount = 0;
static struct Context *PostContext = NULL;
static bool UpdateNumPostponed = false;

/**
 * mutt_num_postponed - Return the number of postponed messages
 * @param m    currently selected mailbox
 * @param force
 * * false Use a cached value if costly to get a fresh count (IMAP)
 * * true Force check
 * @retval num Postponed messages
 */
int mutt_num_postponed(struct Mailbox *m, bool force)
{
  struct stat st;

  static time_t LastModify = 0;
  static char *OldPostponed = NULL;

  if (UpdateNumPostponed)
  {
    UpdateNumPostponed = false;
    force = true;
  }

  if (mutt_str_strcmp(Postponed, OldPostponed) != 0)
  {
    FREE(&OldPostponed);
    OldPostponed = mutt_str_strdup(Postponed);
    LastModify = 0;
    force = true;
  }

  if (!Postponed)
    return 0;

  // We currently are in the Postponed mailbox so just pick the current status
  if (m && mutt_str_strcmp(Postponed, m->realpath) == 0)
  {
    PostCount = m->msg_count - m->msg_deleted;
    return PostCount;
  }

#ifdef USE_IMAP
  /* LastModify is useless for IMAP */
  if (imap_path_probe(Postponed, NULL) == MUTT_IMAP)
  {
    if (force)
    {
      short newpc;

      newpc = imap_path_status(Postponed, false);
      if (newpc >= 0)
      {
        PostCount = newpc;
        mutt_debug(3, "%d postponed IMAP messages found.\n", PostCount);
      }
      else
        mutt_debug(3, "using old IMAP postponed count.\n");
    }
    return PostCount;
  }
#endif

  if (stat(Postponed, &st) == -1)
  {
    PostCount = 0;
    LastModify = 0;
    return 0;
  }

  if (S_ISDIR(st.st_mode))
  {
    /* if we have a maildir mailbox, we need to stat the "new" dir */

    char buf[PATH_MAX];

    snprintf(buf, sizeof(buf), "%s/new", Postponed);
    if (access(buf, F_OK) == 0 && stat(buf, &st) == -1)
    {
      PostCount = 0;
      LastModify = 0;
      return 0;
    }
  }

  if (LastModify < st.st_mtime)
  {
#ifdef USE_NNTP
    int optnews = OptNews;
#endif
    LastModify = st.st_mtime;

    if (access(Postponed, R_OK | F_OK) != 0)
      return PostCount = 0;
#ifdef USE_NNTP
    if (optnews)
      OptNews = false;
#endif
    struct Context *ctx = mx_mbox_open(NULL, Postponed, MUTT_NOSORT | MUTT_QUIET);
    if (!ctx)
      PostCount = 0;
    else
      PostCount = ctx->mailbox->msg_count;
    mx_fastclose_mailbox(ctx->mailbox);
    ctx_free(&ctx);
#ifdef USE_NNTP
    if (optnews)
      OptNews = true;
#endif
  }

  return PostCount;
}

/**
 * mutt_update_num_postponed - Force the update of the number of postponed messages
 */
void mutt_update_num_postponed(void)
{
  UpdateNumPostponed = true;
}

/**
 * post_make_entry - Format a menu item for the email list - Implements Menu::menu_make_entry()
 */
static void post_make_entry(char *buf, size_t buflen, struct Menu *menu, int line)
{
  struct Context *ctx = menu->data;

  mutt_make_string_flags(buf, buflen, NONULL(IndexFormat), ctx,
                         ctx->mailbox->emails[line], MUTT_FORMAT_ARROWCURSOR);
}

/**
 * select_msg - Create a Menu to select a postponed message
 * @retval ptr Email Header
 */
static struct Email *select_msg(void)
{
  int r = -1;
  bool done = false;
  char helpstr[LONG_STRING];

  struct Menu *menu = mutt_menu_new(MENU_POST);
  menu->menu_make_entry = post_make_entry;
  menu->max = PostContext->mailbox->msg_count;
  menu->title = _("Postponed Messages");
  menu->data = PostContext;
  menu->help = mutt_compile_help(helpstr, sizeof(helpstr), MENU_POST, PostponeHelp);
  mutt_menu_push_current(menu);

  /* The postponed mailbox is setup to have sorting disabled, but the global
   * Sort variable may indicate something different.   Sorting has to be
   * disabled while the postpone menu is being displayed. */
  const short orig_sort = Sort;
  Sort = SORT_ORDER;

  while (!done)
  {
    const int i = mutt_menu_loop(menu);
    switch (i)
    {
      case OP_DELETE:
      case OP_UNDELETE:
        /* should deleted draft messages be saved in the trash folder? */
        mutt_set_flag(PostContext->mailbox, PostContext->mailbox->emails[menu->current],
                      MUTT_DELETE, (i == OP_DELETE) ? 1 : 0);
        PostCount = PostContext->mailbox->msg_count - PostContext->mailbox->msg_deleted;
        if (Resolve && menu->current < menu->max - 1)
        {
          menu->oldcurrent = menu->current;
          menu->current++;
          if (menu->current >= menu->top + menu->pagelen)
          {
            menu->top = menu->current;
            menu->redraw |= REDRAW_INDEX | REDRAW_STATUS;
          }
          else
            menu->redraw |= REDRAW_MOTION_RESYNCH;
        }
        else
          menu->redraw |= REDRAW_CURRENT;
        break;

      case OP_GENERIC_SELECT_ENTRY:
        r = menu->current;
        done = true;
        break;

      case OP_EXIT:
        done = true;
        break;
    }
  }

  Sort = orig_sort;
  mutt_menu_pop_current(menu);
  mutt_menu_destroy(&menu);
  return r > -1 ? PostContext->mailbox->emails[r] : NULL;
}

/**
 * mutt_get_postponed - Recall a postponed message
 * @param ctx     Context info, used when recalling a message to which we reply
 * @param hdr     envelope/attachment info for recalled message
 * @param cur     if message was a reply, `cur' is set to the message which `hdr' is in reply to
 * @param fcc     fcc for the recalled message
 * @param fcclen  max length of fcc
 * @retval -1         Error/no messages
 * @retval 0          Normal exit
 * @retval #SEND_REPLY Recalled message is a reply
 */
int mutt_get_postponed(struct Context *ctx, struct Email *hdr,
                       struct Email **cur, char *fcc, size_t fcclen)
{
  struct Email *e = NULL;
  int code = SEND_POSTPONED;
  const char *p = NULL;
  int opt_delete;

  if (!Postponed)
    return -1;

  struct Mailbox *m = mx_mbox_find2(Postponed);
  if (ctx->mailbox == m)
    PostContext = ctx;
  else
    PostContext = mx_mbox_open(m, Postponed, MUTT_NOSORT);

  if (!PostContext)
  {
    PostCount = 0;
    mutt_error(_("No postponed messages"));
    return -1;
  }

  if (!PostContext->mailbox->msg_count)
  {
    PostCount = 0;
    if (PostContext == ctx)
      PostContext = NULL;
    else
      mx_mbox_close(&PostContext);
    mutt_error(_("No postponed messages"));
    return -1;
  }

  if (PostContext->mailbox->msg_count == 1)
  {
    /* only one message, so just use that one. */
    e = PostContext->mailbox->emails[0];
  }
  else if (!(e = select_msg()))
  {
    if (PostContext == ctx)
      PostContext = NULL;
    else
      mx_mbox_close(&PostContext);
    return -1;
  }

  if (mutt_prepare_template(NULL, PostContext->mailbox, hdr, e, false) < 0)
  {
    if (PostContext != ctx)
    {
      mx_fastclose_mailbox(PostContext->mailbox);
      FREE(&PostContext);
    }
    return -1;
  }

  /* finished with this message, so delete it. */
  mutt_set_flag(PostContext->mailbox, e, MUTT_DELETE, 1);
  mutt_set_flag(PostContext->mailbox, e, MUTT_PURGE, 1);

  /* update the count for the status display */
  PostCount = PostContext->mailbox->msg_count - PostContext->mailbox->msg_deleted;

  /* avoid the "purge deleted messages" prompt */
  opt_delete = Delete;
  Delete = MUTT_YES;
  if (PostContext == ctx)
    PostContext = NULL;
  else
    mx_mbox_close(&PostContext);
  Delete = opt_delete;

  struct ListNode *np, *tmp;
  STAILQ_FOREACH_SAFE(np, &hdr->env->userhdrs, entries, tmp)
  {
    size_t plen = mutt_str_startswith(np->data, "X-Mutt-References:", CASE_IGNORE);
    if (plen)
    {
      if (ctx)
      {
        /* if a mailbox is currently open, look to see if the original message
           the user attempted to reply to is in this mailbox */
        p = mutt_str_skip_email_wsp(np->data + plen);
        if (!ctx->mailbox->id_hash)
          ctx->mailbox->id_hash = mutt_make_id_hash(ctx->mailbox);
        *cur = mutt_hash_find(ctx->mailbox->id_hash, p);
      }
      if (*cur)
        code |= SEND_REPLY;
    }
    else if ((plen = mutt_str_startswith(np->data, "X-Mutt-Fcc:", CASE_IGNORE)))
    {
      p = mutt_str_skip_email_wsp(np->data + plen);
      mutt_str_strfcpy(fcc, p, fcclen);
      mutt_pretty_mailbox(fcc, fcclen);

      /* note that x-mutt-fcc was present.  we do this because we want to add a
       * default fcc if the header was missing, but preserve the request of the
       * user to not make a copy if the header field is present, but empty.
       * see http://dev.mutt.org/trac/ticket/3653
       */
      code |= SEND_POSTPONED_FCC;
    }
    else if (((WithCrypto & APPLICATION_PGP) != 0) &&
             /* this is generated by old neomutt versions */
             (mutt_str_startswith(np->data, "Pgp:", CASE_MATCH) ||
              /* this is the new way */
              mutt_str_startswith(np->data, "X-Mutt-PGP:", CASE_MATCH)))
    {
      hdr->security = mutt_parse_crypt_hdr(strchr(np->data, ':') + 1, 1, APPLICATION_PGP);
      hdr->security |= APPLICATION_PGP;
    }
    else if (((WithCrypto & APPLICATION_SMIME) != 0) &&
             mutt_str_startswith(np->data, "X-Mutt-SMIME:", CASE_MATCH))
    {
      hdr->security = mutt_parse_crypt_hdr(strchr(np->data, ':') + 1, 1, APPLICATION_SMIME);
      hdr->security |= APPLICATION_SMIME;
    }

#ifdef MIXMASTER
    else if (mutt_str_startswith(np->data, "X-Mutt-Mix:", CASE_MATCH))
    {
      mutt_list_free(&hdr->chain);

      char *t = strtok(np->data + 11, " \t\n");
      while (t)
      {
        mutt_list_insert_tail(&hdr->chain, mutt_str_strdup(t));
        t = strtok(NULL, " \t\n");
      }
    }
#endif

    else
    {
      // skip header removal
      continue;
    }

    // remove the header
    STAILQ_REMOVE(&hdr->env->userhdrs, np, ListNode, entries);
    FREE(&np->data);
    FREE(&np);
  }

  if (CryptOpportunisticEncrypt)
    crypt_opportunistic_encrypt(hdr);

  return code;
}

/**
 * mutt_parse_crypt_hdr - Parse a crypto header string
 * @param p                Header string to parse
 * @param set_empty_signas Allow an empty "Sign as"
 * @param crypt_app App, e.g. #APPLICATION_PGP
 * @retval num Flags, e.g. #ENCRYPT
 */
int mutt_parse_crypt_hdr(const char *p, int set_empty_signas, int crypt_app)
{
  char smime_cryptalg[LONG_STRING] = "\0";
  char sign_as[LONG_STRING] = "\0", *q = NULL;
  int flags = 0;

  if (!WithCrypto)
    return 0;

  p = mutt_str_skip_email_wsp(p);
  for (; *p; p++)
  {
    switch (*p)
    {
      case 'c':
      case 'C':
        q = smime_cryptalg;

        if (*(p + 1) == '<')
        {
          for (p += 2; *p && (*p != '>') &&
                       (q < (smime_cryptalg + sizeof(smime_cryptalg) - 1));
               *q++ = *p++)
          {
          }

          if (*p != '>')
          {
            mutt_error(_("Illegal S/MIME header"));
            return 0;
          }
        }

        *q = '\0';
        break;

      case 'e':
      case 'E':
        flags |= ENCRYPT;
        break;

      case 'i':
      case 'I':
        flags |= INLINE;
        break;

      /* This used to be the micalg parameter.
       *
       * It's no longer needed, so we just skip the parameter in order
       * to be able to recall old messages.
       */
      case 'm':
      case 'M':
        if (*(p + 1) == '<')
        {
          for (p += 2; *p && (*p != '>'); p++)
            ;
          if (*p != '>')
          {
            mutt_error(_("Illegal crypto header"));
            return 0;
          }
        }

        break;

      case 'o':
      case 'O':
        flags |= OPPENCRYPT;
        break;

      case 's':
      case 'S':
        flags |= SIGN;
        q = sign_as;

        if (*(p + 1) == '<')
        {
          for (p += 2; *p && (*p != '>') && (q < (sign_as + sizeof(sign_as) - 1));
               *q++ = *p++)
          {
          }

          if (*p != '>')
          {
            mutt_error(_("Illegal crypto header"));
            return 0;
          }
        }

        *q = '\0';
        break;

      default:
        mutt_error(_("Illegal crypto header"));
        return 0;
    }
  }

  /* the cryptalg field must not be empty */
  if (((WithCrypto & APPLICATION_SMIME) != 0) && *smime_cryptalg)
    mutt_str_replace(&SmimeEncryptWith, smime_cryptalg);

  /* Set {Smime,Pgp}SignAs, if desired. */

  if (((WithCrypto & APPLICATION_PGP) != 0) && (crypt_app == APPLICATION_PGP) &&
      (flags & SIGN) && (set_empty_signas || *sign_as))
  {
    mutt_str_replace(&PgpSignAs, sign_as);
  }

  if (((WithCrypto & APPLICATION_SMIME) != 0) && (crypt_app == APPLICATION_SMIME) &&
      (flags & SIGN) && (set_empty_signas || *sign_as))
  {
    mutt_str_replace(&SmimeSignAs, sign_as);
  }

  return flags;
}

/**
 * mutt_prepare_template - Prepare a message template
 * @param fp      If not NULL, file containing the template
 * @param m       If fp is NULL, the Mailbox containing the header with the template
 * @param newhdr  The template is read into this Header
 * @param e       Email to recall/resend
 * @param resend  Set if resending (as opposed to recalling a postponed msg)
 *                Resent messages enable header weeding, and also
 *                discard any existing Message-ID and Mail-Followup-To
 * @retval  0 Success
 * @retval -1 Error
 */
int mutt_prepare_template(FILE *fp, struct Mailbox *m, struct Email *newhdr,
                          struct Email *e, bool resend)
{
  struct Message *msg = NULL;
  char file[PATH_MAX];
  struct Body *b = NULL;
  FILE *bfp = NULL;
  int rc = -1;
  struct State s = { 0 };
  int sec_type;

  if (!fp && !(msg = mx_msg_open(m, e->msgno)))
    return -1;

  if (!fp)
    fp = msg->fp;

  bfp = fp;

  /* parse the message header and MIME structure */

  fseeko(fp, e->offset, SEEK_SET);
  newhdr->offset = e->offset;
  /* enable header weeding for resent messages */
  newhdr->env = mutt_rfc822_read_header(fp, newhdr, true, resend);
  newhdr->content->length = e->content->length;
  mutt_parse_part(fp, newhdr->content);

  /* If resending a message, don't keep message_id or mail_followup_to.
   * Otherwise, we are resuming a postponed message, and want to keep those
   * headers if they exist.
   */
  if (resend)
  {
    FREE(&newhdr->env->message_id);
    FREE(&newhdr->env->mail_followup_to);
  }

  /* decrypt pgp/mime encoded messages */

  if (((WithCrypto & APPLICATION_PGP) != 0) &&
      (sec_type = mutt_is_multipart_encrypted(newhdr->content)))
  {
    newhdr->security |= sec_type;
    if (!crypt_valid_passphrase(sec_type))
      goto bail;

    mutt_message(_("Decrypting message..."));
    if ((crypt_pgp_decrypt_mime(fp, &bfp, newhdr->content, &b) == -1) || !b)
    {
      goto bail;
    }

    mutt_body_free(&newhdr->content);
    newhdr->content = b;

    mutt_clear_error();
  }

  /* remove a potential multipart/signed layer - useful when
   * resending messages
   */
  if ((WithCrypto != 0) && mutt_is_multipart_signed(newhdr->content))
  {
    newhdr->security |= SIGN;
    if (((WithCrypto & APPLICATION_PGP) != 0) &&
        (mutt_str_strcasecmp(
             mutt_param_get(&newhdr->content->parameter, "protocol"),
             "application/pgp-signature") == 0))
    {
      newhdr->security |= APPLICATION_PGP;
    }
    else if (WithCrypto & APPLICATION_SMIME)
      newhdr->security |= APPLICATION_SMIME;

    /* destroy the signature */
    mutt_body_free(&newhdr->content->parts->next);
    newhdr->content = mutt_remove_multipart(newhdr->content);
  }

  /* We don't need no primary multipart.
   * Note: We _do_ preserve messages!
   *
   * XXX - we don't handle multipart/alternative in any
   * smart way when sending messages.  However, one may
   * consider this a feature.
   */
  if (newhdr->content->type == TYPE_MULTIPART)
    newhdr->content = mutt_remove_multipart(newhdr->content);

  s.fpin = bfp;

  /* create temporary files for all attachments */
  for (b = newhdr->content; b; b = b->next)
  {
    /* what follows is roughly a receive-mode variant of
     * mutt_get_tmp_attachment () from muttlib.c
     */

    file[0] = '\0';
    if (b->filename)
    {
      mutt_str_strfcpy(file, b->filename, sizeof(file));
      b->d_filename = mutt_str_strdup(b->filename);
    }
    else
    {
      /* avoid Content-Disposition: header with temporary filename */
      b->use_disp = false;
    }

    /* set up state flags */

    s.flags = 0;

    if (b->type == TYPE_TEXT)
    {
      if (mutt_str_strcasecmp("yes",
                              mutt_param_get(&b->parameter, "x-mutt-noconv")) == 0)
      {
        b->noconv = true;
      }
      else
      {
        s.flags |= MUTT_CHARCONV;
        b->noconv = false;
      }

      mutt_param_delete(&b->parameter, "x-mutt-noconv");
    }

    mutt_adv_mktemp(file, sizeof(file));
    s.fpout = mutt_file_fopen(file, "w");
    if (!s.fpout)
      goto bail;

    if (((WithCrypto & APPLICATION_PGP) != 0) &&
        ((sec_type = mutt_is_application_pgp(b)) & (ENCRYPT | SIGN)))
    {
      if (sec_type & ENCRYPT)
      {
        if (!crypt_valid_passphrase(APPLICATION_PGP))
          goto bail;
        mutt_message(_("Decrypting message..."));
      }

      if (mutt_body_handler(b, &s) < 0)
      {
        mutt_error(_("Decryption failed"));
        goto bail;
      }

      newhdr->security |= sec_type;

      b->type = TYPE_TEXT;
      mutt_str_replace(&b->subtype, "plain");
      mutt_param_delete(&b->parameter, "x-action");
    }
    else if (((WithCrypto & APPLICATION_SMIME) != 0) &&
             ((sec_type = mutt_is_application_smime(b)) & (ENCRYPT | SIGN)))
    {
      if (sec_type & ENCRYPT)
      {
        if (!crypt_valid_passphrase(APPLICATION_SMIME))
          goto bail;
        crypt_smime_getkeys(newhdr->env);
        mutt_message(_("Decrypting message..."));
      }

      if (mutt_body_handler(b, &s) < 0)
      {
        mutt_error(_("Decryption failed"));
        goto bail;
      }

      newhdr->security |= sec_type;
      b->type = TYPE_TEXT;
      mutt_str_replace(&b->subtype, "plain");
    }
    else
      mutt_decode_attachment(b, &s);

    if (mutt_file_fclose(&s.fpout) != 0)
      goto bail;

    mutt_str_replace(&b->filename, file);
    b->unlink = true;

    mutt_stamp_attachment(b);

    mutt_body_free(&b->parts);
    if (b->email)
      b->email->content = NULL; /* avoid dangling pointer */
  }

  /* Fix encryption flags. */

  /* No inline if multipart. */
  if ((WithCrypto != 0) && (newhdr->security & INLINE) && newhdr->content->next)
    newhdr->security &= ~INLINE;

  /* Do we even support multiple mechanisms? */
  newhdr->security &= WithCrypto | ~(APPLICATION_PGP | APPLICATION_SMIME);

  /* Theoretically, both could be set. Take the one the user wants to set by default. */
  if ((newhdr->security & APPLICATION_PGP) && (newhdr->security & APPLICATION_SMIME))
  {
    if (SmimeIsDefault)
      newhdr->security &= ~APPLICATION_PGP;
    else
      newhdr->security &= ~APPLICATION_SMIME;
  }

  rc = 0;

bail:

  /* that's it. */
  if (bfp != fp)
    mutt_file_fclose(&bfp);
  if (msg)
    mx_msg_close(m, &msg);

  if (rc == -1)
  {
    mutt_env_free(&newhdr->env);
    mutt_body_free(&newhdr->content);
  }

  return rc;
}
