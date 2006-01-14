/* $Id$ */
/*-
 * Copyright (c) 2005 Benedikt Meurer <benny@xfce.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59 Temple
 * Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __THUNAR_HISTORY_H__
#define __THUNAR_HISTORY_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS;

typedef struct _ThunarHistoryClass ThunarHistoryClass;
typedef struct _ThunarHistory      ThunarHistory;

#define THUNAR_TYPE_HISTORY             (thunar_history_get_type ())
#define THUNAR_HISTORY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNAR_TYPE_HISTORY, ThunarHistory))
#define THUNAR_HISTORY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), THUNAR_TYPE_HISTORY, ThunarHistoryClass))
#define THUNAR_IS_HISTORY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNAR_TYPE_HISTORY))
#define THUNAR_IS_HISTORY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), THUNAR_TYPE_HISTORY))
#define THUNAR_HISTORY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), THUNAR_TYPE_HISTORY, ThunarHistoryClass))

GType           thunar_history_get_type         (void) G_GNUC_CONST;

ThunarHistory  *thunar_history_new              (void) G_GNUC_MALLOC;

GtkActionGroup *thunar_history_get_action_group (const ThunarHistory *history);
void            thunar_history_set_action_group (ThunarHistory       *history,
                                                 GtkActionGroup      *action_group);

G_END_DECLS;

#endif /* !__THUNAR_HISTORY_H__ */