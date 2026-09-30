/** \file mcterm_far2l.h
 *  \brief Header: mcterm as a far2l terminal, for the programs that run in it
 *
 *  A program in mc's terminal that speaks the far2l extensions (f4, far2l, another mc) can be
 *  given the drag and drop of the terminal mc itself runs in: a file dropped on the window of
 *  the outer far2l terminal reaches mc, and mc passes it on to the program as a drop on its
 *  own terminal.  Each hop is an application for the offer of the one outside it and the
 *  terminal for the one inside: the program's LIST and READ are answered with LIST and READ of
 *  the outer offer, chunk by chunk, with nothing kept in between.
 *
 *  The extensions are answered only while there is something to pass on (the outer terminal
 *  has them on and drop reception is bound): then the program also expects its keys as far2l
 *  events, which is what mcterm_far2l_key gives.
 */

#ifndef MC__MCTERM_FAR2L_H
#define MC__MCTERM_FAR2L_H

#include "lib/global.h"
#include "lib/tty/far2l.h"

/*** typedefs(not structures) and defined constants **********************************************/

/* the outer terminal's side, replaceable so that the terminal can be tested without one */
typedef struct
{
    gboolean (*is_bound) (void);
    int (*list) (const guint8 *offer, GPtrArray *entries);
    int (*read) (const guint8 *offer, guint64 item_id, guint64 offset, guint32 length,
                 guint8 **data, gsize *data_len, gboolean *eof);
    void (*close) (const guint8 *offer, guint8 reason);
    guint32 (*max_chunk) (void);
} mcterm_far2l_parent_t;

typedef struct mcterm_far2l_struct mcterm_far2l_t;

/*** declarations of public functions ************************************************************/

/* parent NULL: the terminal mc runs in */
mcterm_far2l_t *mcterm_far2l_new (const mcterm_far2l_parent_t *parent);
/* Gives every offer back to the outer terminal, cancelled. */
void mcterm_far2l_free (mcterm_far2l_t *f);

/* The program has switched the extensions on and been answered. */
gboolean mcterm_far2l_on (const mcterm_far2l_t *f);
/* The program has bound drop reception: a drop is worth passing on. */
gboolean mcterm_far2l_bound (const mcterm_far2l_t *f);

/* One APC of the program, what lies between ESC _ and its terminator.  What is to be written
   to the program comes back as text (g_free() it), NULL for nothing. */
char *mcterm_far2l_apc (mcterm_far2l_t *f, const char *body, gsize len);

/* A drop on the outer terminal landed on this terminal at the cell x, y of it (-1, -1 when
   unknown).  TRUE: the terminal has taken the offer, which it will close, and the event to
   write to the program is in out; FALSE: not taken, the offer is the caller's. */
gboolean mcterm_far2l_drop (mcterm_far2l_t *f, const far2l_drop_t *drop, int x, int y, char **out);

/* A key of mc as far2l key events.  TRUE: it is dealt with (out may stay NULL while a
   character of several bytes is not complete yet); FALSE: the caller sends it as bytes. */
gboolean mcterm_far2l_key (mcterm_far2l_t *f, int key, char **out);

#endif
