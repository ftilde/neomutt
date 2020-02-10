/**
 * @file
 * ConnAccount object used by POP and IMAP
 *
 * @authors
 * Copyright (C) 2000-2007 Brendan Cully <brendan@kublai.com>
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
 * @page mutt_account ConnAccount object used by POP and IMAP
 *
 * ConnAccount object used by POP and IMAP
 */

#include "config.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "mutt/mutt.h"
#include "email/lib.h"
#include "conn/conn.h"
#include "gui/lib.h"
#include "mutt.h"
#include "mutt_account.h"
#include "globals.h"
#include "options.h"

/* These Config Variables are only used in mutt_account.c */
char *C_ImapLogin; ///< Config: (imap) Login name for the IMAP server (defaults to #C_ImapUser)
char *C_ImapOauthRefreshCommand; ///< Config: (imap) External command to generate OAUTH refresh token
char *C_ImapPass; ///< Config: (imap) Password for the IMAP server
char *C_NntpPass; ///< Config: (nntp) Password for the news server
char *C_NntpUser; ///< Config: (nntp) Username for the news server
char *C_PopOauthRefreshCommand; ///< Config: (pop) External command to generate OAUTH refresh token
char *C_PopPass; ///< Config: (pop) Password of the POP server
char *C_PopUser; ///< Config: (pop) Username of the POP server
char *C_SmtpOauthRefreshCommand; ///< Config: (smtp) External command to generate OAUTH refresh token
char *C_SmtpPass; ///< Config: (smtp) Password for the SMTP server
char *C_SmtpUser; ///< Config: (smtp) Username for the SMTP server

/**
 * mutt_account_fromuri - Fill ConnAccount with information from uri
 * @param account ConnAccount to fill
 * @param uri     Uri to parse
 * @retval  0 Success
 * @retval -1 Error
 */
int mutt_account_fromuri(struct ConnAccount *account, const struct Uri *uri)
{
  /* must be present */
  if (uri->host)
    mutt_str_strfcpy(account->host, uri->host, sizeof(account->host));
  else
    return -1;

  if (uri->user)
  {
    mutt_str_strfcpy(account->user, uri->user, sizeof(account->user));
    account->flags |= MUTT_ACCT_USER;
  }
  if (uri->pass)
  {
    mutt_str_strfcpy(account->pass, uri->pass, sizeof(account->pass));
    account->flags |= MUTT_ACCT_PASS;
  }
  if (uri->port)
  {
    account->port = uri->port;
    account->flags |= MUTT_ACCT_PORT;
  }

  return 0;
}

/**
 * mutt_account_touri - Fill URI with info from account
 * @param account Source ConnAccount
 * @param uri     Uri to fill
 *
 * The URI information is a set of pointers into account - don't free or edit
 * account until you've finished with uri (make a copy of account if you need
 * it for a while).
 */
void mutt_account_touri(struct ConnAccount *account, struct Uri *uri)
{
  uri->scheme = U_UNKNOWN;
  uri->user = NULL;
  uri->pass = NULL;
  uri->port = 0;
  uri->path = NULL;

#ifdef USE_IMAP
  if (account->type == MUTT_ACCT_TYPE_IMAP)
  {
    if (account->flags & MUTT_ACCT_SSL)
      uri->scheme = U_IMAPS;
    else
      uri->scheme = U_IMAP;
  }
#endif

#ifdef USE_POP
  if (account->type == MUTT_ACCT_TYPE_POP)
  {
    if (account->flags & MUTT_ACCT_SSL)
      uri->scheme = U_POPS;
    else
      uri->scheme = U_POP;
  }
#endif

#ifdef USE_SMTP
  if (account->type == MUTT_ACCT_TYPE_SMTP)
  {
    if (account->flags & MUTT_ACCT_SSL)
      uri->scheme = U_SMTPS;
    else
      uri->scheme = U_SMTP;
  }
#endif

#ifdef USE_NNTP
  if (account->type == MUTT_ACCT_TYPE_NNTP)
  {
    if (account->flags & MUTT_ACCT_SSL)
      uri->scheme = U_NNTPS;
    else
      uri->scheme = U_NNTP;
  }
#endif

  uri->host = account->host;
  if (account->flags & MUTT_ACCT_PORT)
    uri->port = account->port;
  if (account->flags & MUTT_ACCT_USER)
    uri->user = account->user;
  if (account->flags & MUTT_ACCT_PASS)
    uri->pass = account->pass;
}

/**
 * mutt_account_getuser - Retrieve username into ConnAccount, if necessary
 * @param account ConnAccount to fill
 * @retval  0 Success
 * @retval -1 Failure
 */
int mutt_account_getuser(struct ConnAccount *account)
{
  char prompt[256];

  /* already set */
  if (account->flags & MUTT_ACCT_USER)
    return 0;
#ifdef USE_IMAP
  else if ((account->type == MUTT_ACCT_TYPE_IMAP) && C_ImapUser)
    mutt_str_strfcpy(account->user, C_ImapUser, sizeof(account->user));
#endif
#ifdef USE_POP
  else if ((account->type == MUTT_ACCT_TYPE_POP) && C_PopUser)
    mutt_str_strfcpy(account->user, C_PopUser, sizeof(account->user));
#endif
#ifdef USE_NNTP
  else if ((account->type == MUTT_ACCT_TYPE_NNTP) && C_NntpUser)
    mutt_str_strfcpy(account->user, C_NntpUser, sizeof(account->user));
#endif
#ifdef USE_SMTP
  else if ((account->type == MUTT_ACCT_TYPE_SMTP) && C_SmtpUser)
    mutt_str_strfcpy(account->user, C_SmtpUser, sizeof(account->user));
#endif
  else if (OptNoCurses)
    return -1;
  /* prompt (defaults to unix username), copy into account->user */
  else
  {
    /* L10N: Example: Username at myhost.com */
    snprintf(prompt, sizeof(prompt), _("Username at %s: "), account->host);
    mutt_str_strfcpy(account->user, Username, sizeof(account->user));
    if (mutt_get_field_unbuffered(prompt, account->user, sizeof(account->user), MUTT_COMP_NO_FLAGS))
      return -1;
  }

  account->flags |= MUTT_ACCT_USER;

  return 0;
}

/**
 * mutt_account_getlogin - Retrieve login info into ConnAccount, if necessary
 * @param account ConnAccount to fill
 * @retval  0 Success
 * @retval -1 Failure
 */
int mutt_account_getlogin(struct ConnAccount *account)
{
  /* already set */
  if (account->flags & MUTT_ACCT_LOGIN)
    return 0;
#ifdef USE_IMAP
  if (account->type == MUTT_ACCT_TYPE_IMAP)
  {
    if (C_ImapLogin)
    {
      mutt_str_strfcpy(account->login, C_ImapLogin, sizeof(account->login));
      account->flags |= MUTT_ACCT_LOGIN;
    }
  }
#endif

  if (!(account->flags & MUTT_ACCT_LOGIN))
  {
    if (mutt_account_getuser(account) == 0)
    {
      mutt_str_strfcpy(account->login, account->user, sizeof(account->login));
      account->flags |= MUTT_ACCT_LOGIN;
    }
    else
    {
      mutt_debug(LL_DEBUG1, "Couldn't get user info\n");
      return -1;
    }
  }

  return 0;
}

/**
 * mutt_account_getpass - Fetch password into ConnAccount, if necessary
 * @param account ConnAccount to fill
 * @retval  0 Success
 * @retval -1 Failure
 */
int mutt_account_getpass(struct ConnAccount *account)
{
  char prompt[256];

  if (account->flags & MUTT_ACCT_PASS)
    return 0;
#ifdef USE_IMAP
  else if ((account->type == MUTT_ACCT_TYPE_IMAP) && C_ImapPass)
    mutt_str_strfcpy(account->pass, C_ImapPass, sizeof(account->pass));
#endif
#ifdef USE_POP
  else if ((account->type == MUTT_ACCT_TYPE_POP) && C_PopPass)
    mutt_str_strfcpy(account->pass, C_PopPass, sizeof(account->pass));
#endif
#ifdef USE_SMTP
  else if ((account->type == MUTT_ACCT_TYPE_SMTP) && C_SmtpPass)
    mutt_str_strfcpy(account->pass, C_SmtpPass, sizeof(account->pass));
#endif
#ifdef USE_NNTP
  else if ((account->type == MUTT_ACCT_TYPE_NNTP) && C_NntpPass)
    mutt_str_strfcpy(account->pass, C_NntpPass, sizeof(account->pass));
#endif
  else if (OptNoCurses)
    return -1;
  else
  {
    snprintf(prompt, sizeof(prompt), _("Password for %s@%s: "),
             (account->flags & MUTT_ACCT_LOGIN) ? account->login : account->user,
             account->host);
    account->pass[0] = '\0';
    if (mutt_get_password(prompt, account->pass, sizeof(account->pass)))
      return -1;
  }

  account->flags |= MUTT_ACCT_PASS;

  return 0;
}

/**
 * mutt_account_unsetpass - Unset ConnAccount's password
 * @param account ConnAccount to modify
 */
void mutt_account_unsetpass(struct ConnAccount *account)
{
  account->flags &= ~MUTT_ACCT_PASS;
}

/**
 * mutt_account_getoauthbearer - Get an OAUTHBEARER token
 * @param account Account to use
 * @retval ptr  OAuth token
 * @retval NULL Error
 *
 * Run an external command to generate the oauth refresh token for an account,
 * then create and encode the OAUTHBEARER token based on RFC7628.
 *
 * @note Caller should free the token
 */
char *mutt_account_getoauthbearer(struct ConnAccount *account)
{
  FILE *fp = NULL;
  char *cmd = NULL;
  char *token = NULL;
  size_t token_size = 0;
  char *oauthbearer = NULL;
  size_t oalen;
  char *encoded_token = NULL;
  size_t encoded_len;
  pid_t pid;

  /* The oauthbearer token includes the login */
  if (mutt_account_getlogin(account))
    return NULL;

#ifdef USE_IMAP
  if ((account->type == MUTT_ACCT_TYPE_IMAP) && C_ImapOauthRefreshCommand)
    cmd = C_ImapOauthRefreshCommand;
#endif
#ifdef USE_POP
  else if ((account->type == MUTT_ACCT_TYPE_POP) && C_PopOauthRefreshCommand)
    cmd = C_PopOauthRefreshCommand;
#endif
#ifdef USE_SMTP
  else if ((account->type == MUTT_ACCT_TYPE_SMTP) && C_SmtpOauthRefreshCommand)
    cmd = C_SmtpOauthRefreshCommand;
#endif

  if (!cmd)
  {
    /* L10N: You will see this error message if (1) you have "oauthbearer" in
       one of your $*_authenticators and (2) you do not have the corresponding
       $*_oauth_refresh_command defined. So the message does not mean "None of
       your $*_oauth_refresh_command's are defined." */
    mutt_error(_("No OAUTH refresh command defined"));
    return NULL;
  }

  pid = filter_create(cmd, NULL, &fp, NULL);
  if (pid < 0)
  {
    mutt_perror(_("Unable to run refresh command"));
    return NULL;
  }

  token = mutt_file_read_line(NULL, &token_size, fp, NULL, 0);
  mutt_file_fclose(&fp);
  filter_wait(pid);

  if (!token || (*token == '\0'))
  {
    mutt_error(_("Command returned empty string"));
    FREE(&token);
    return NULL;
  }

  /* Determine the length of the keyed message digest, add 50 for overhead. */
  oalen = strlen(account->login) + strlen(account->host) + strlen(token) + 50;
  oauthbearer = mutt_mem_malloc(oalen);

  snprintf(oauthbearer, oalen, "n,a=%s,\001host=%s\001port=%d\001auth=Bearer %s\001\001",
           account->login, account->host, account->port, token);

  FREE(&token);

  encoded_len = strlen(oauthbearer) * 4 / 3 + 10;
  encoded_token = mutt_mem_malloc(encoded_len);
  mutt_b64_encode(oauthbearer, strlen(oauthbearer), encoded_token, encoded_len);
  FREE(&oauthbearer);
  return encoded_token;
}
