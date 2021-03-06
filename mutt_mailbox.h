#ifndef MUTT_MUTT_MAILBOX_H
#define MUTT_MUTT_MAILBOX_H

#include <stdbool.h>

struct Buffer;
struct Mailbox;
struct stat;

/* force flags passed to mutt_mailbox_check() */
#define MUTT_MAILBOX_CHECK_FORCE       (1 << 0)
#define MUTT_MAILBOX_CHECK_FORCE_STATS (1 << 1)

int  mutt_mailbox_check       (struct Mailbox *m_cur, int force);
void mutt_mailbox_cleanup     (const char *path, struct stat *st);
bool mutt_mailbox_list        (void);
struct Mailbox *mutt_mailbox_next(struct Mailbox *m_cur, struct Buffer *s);
bool mutt_mailbox_notify      (struct Mailbox *m_cur);
void mutt_mailbox_set_notified(struct Mailbox *m);

#endif /* MUTT_MUTT_MAILBOX_H */
