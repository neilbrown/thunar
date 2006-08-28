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

#ifndef __THUNAR_PATH_BAR_H__
#define __THUNAR_PATH_BAR_H__

#include <thunar/thunar-navigator.h>

G_BEGIN_DECLS;

typedef struct _ThunarPathBarClass ThunarPathBarClass;
typedef struct _ThunarPathBar      ThunarPathBar;

#define THUNAR_TYPE_PATH_BAR            (thunar_path_bar_get_type ())
#define THUNAR_PATH_BAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), THUNAR_TYPE_PATH_BAR, ThunarPathBar))
#define THUNAR_PATH_BAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), THUNAR_TYPE_PATH_BAR, ThunarPathBarClass))
#define THUNAR_IS_PATH_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), THUNAR_TYPE_PATH_BAR))
#define THUNAR_IS_PATH_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), THUNAR_TYPE_PATH_BAR))
#define THUNAR_PATH_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), THUNAR_TYPE_PATH_BAR, ThunarPathBarClass))

GType      thunar_path_bar_get_type     (void) G_GNUC_CONST;

GtkWidget *thunar_path_bar_new          (void) G_GNUC_MALLOC G_GNUC_WARN_UNUSED_RESULT;

void       thunar_path_bar_down_folder  (ThunarPathBar *path_bar);

G_END_DECLS;

#endif /* !__THUNAR_PATH_BAR_H__ */