/* $Id$ */
/*-
 * Copyright (c) 2005-2006 Benedikt Meurer <benny@xfce.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_SYS_CDIO_H
#include <sys/cdio.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_FSTAB_H
#include <fstab.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <exo/exo.h>

#include <thunar-vfs/thunar-vfs-volume-freebsd.h>
#include <thunar-vfs/thunar-vfs-alias.h>



static void                     thunar_vfs_volume_freebsd_class_init       (ThunarVfsVolumeFreeBSDClass *klass);
static void                     thunar_vfs_volume_freebsd_volume_init      (ThunarVfsVolumeIface        *iface);
static void                     thunar_vfs_volume_freebsd_init             (ThunarVfsVolumeFreeBSD      *volume_freebsd);
static void                     thunar_vfs_volume_freebsd_finalize         (GObject                     *object);
static ThunarVfsVolumeKind      thunar_vfs_volume_freebsd_get_kind         (ThunarVfsVolume             *volume);
static const gchar             *thunar_vfs_volume_freebsd_get_name         (ThunarVfsVolume             *volume);
static ThunarVfsVolumeStatus    thunar_vfs_volume_freebsd_get_status       (ThunarVfsVolume             *volume);
static ThunarVfsPath           *thunar_vfs_volume_freebsd_get_mount_point  (ThunarVfsVolume             *volume);
static gboolean                 thunar_vfs_volume_freebsd_eject            (ThunarVfsVolume             *volume,
                                                                            GtkWidget                   *window,
                                                                            GError                     **error);
static gboolean                 thunar_vfs_volume_freebsd_mount            (ThunarVfsVolume             *volume,
                                                                            GtkWidget                   *window,
                                                                            GError                     **error);
static gboolean                 thunar_vfs_volume_freebsd_unmount          (ThunarVfsVolume             *volume,
                                                                            GtkWidget                   *window,
                                                                            GError                     **error);
static gboolean                 thunar_vfs_volume_freebsd_update           (gpointer                     user_data);
static ThunarVfsVolumeFreeBSD  *thunar_vfs_volume_freebsd_new              (const gchar                 *device_path,
                                                                            const gchar                 *mount_path);



struct _ThunarVfsVolumeFreeBSDClass
{
  GObjectClass __parent__;
};

struct _ThunarVfsVolumeFreeBSD
{
  GObject __parent__;

  gchar                *device_path;
  const gchar          *device_name;
  ThunarVfsFileDevice   device_id;

  gchar                *label;

  struct statfs         info;
  ThunarVfsPath        *mount_point;

  ThunarVfsVolumeKind   kind;
  ThunarVfsVolumeStatus status;

  gint                  update_timer_id;
};



G_DEFINE_TYPE_WITH_CODE (ThunarVfsVolumeFreeBSD,
                         thunar_vfs_volume_freebsd,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (THUNAR_VFS_TYPE_VOLUME,
                                                thunar_vfs_volume_freebsd_volume_init));



static void
thunar_vfs_volume_freebsd_class_init (ThunarVfsVolumeFreeBSDClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = thunar_vfs_volume_freebsd_finalize;
}



static void
thunar_vfs_volume_freebsd_volume_init (ThunarVfsVolumeIface *iface)
{
  iface->get_kind = thunar_vfs_volume_freebsd_get_kind;
  iface->get_name = thunar_vfs_volume_freebsd_get_name;
  iface->get_status = thunar_vfs_volume_freebsd_get_status;
  iface->get_mount_point = thunar_vfs_volume_freebsd_get_mount_point;
  iface->eject = thunar_vfs_volume_freebsd_eject;
  iface->mount = thunar_vfs_volume_freebsd_mount;
  iface->unmount = thunar_vfs_volume_freebsd_unmount;
}



static void
thunar_vfs_volume_freebsd_init (ThunarVfsVolumeFreeBSD *volume_freebsd)
{
}



static void
thunar_vfs_volume_freebsd_finalize (GObject *object)
{
  ThunarVfsVolumeFreeBSD *volume_freebsd = THUNAR_VFS_VOLUME_FREEBSD (object);

  g_return_if_fail (THUNAR_VFS_IS_VOLUME_FREEBSD (volume_freebsd));

  if (G_LIKELY (volume_freebsd->update_timer_id >= 0))
    g_source_remove (volume_freebsd->update_timer_id);

  if (G_LIKELY (volume_freebsd->mount_point != NULL))
    thunar_vfs_path_unref (volume_freebsd->mount_point);

  g_free (volume_freebsd->device_path);
  g_free (volume_freebsd->label);

  G_OBJECT_CLASS (thunar_vfs_volume_freebsd_parent_class)->finalize (object);
}



static ThunarVfsVolumeKind
thunar_vfs_volume_freebsd_get_kind (ThunarVfsVolume *volume)
{
  return THUNAR_VFS_VOLUME_FREEBSD (volume)->kind;
}



static const gchar*
thunar_vfs_volume_freebsd_get_name (ThunarVfsVolume *volume)
{
  ThunarVfsVolumeFreeBSD *volume_freebsd = THUNAR_VFS_VOLUME_FREEBSD (volume);

  return (volume_freebsd->label != NULL) ? volume_freebsd->label : volume_freebsd->device_name;
}



static ThunarVfsVolumeStatus
thunar_vfs_volume_freebsd_get_status (ThunarVfsVolume *volume)
{
  return THUNAR_VFS_VOLUME_FREEBSD (volume)->status;
}



static ThunarVfsPath*
thunar_vfs_volume_freebsd_get_mount_point (ThunarVfsVolume *volume)
{
  return THUNAR_VFS_VOLUME_FREEBSD (volume)->mount_point;
}



static gboolean
thunar_vfs_volume_freebsd_eject (ThunarVfsVolume *volume,
                                 GtkWidget       *window,
                                 GError         **error)
{
  ThunarVfsVolumeFreeBSD *volume_freebsd = THUNAR_VFS_VOLUME_FREEBSD (volume);
  gboolean                result;
  gchar                  *standard_error;
  gchar                  *command_line;
  gchar                  *quoted;
  gint                    exit_status;

  /* generate the command line for the eject command */
  quoted = g_shell_quote (volume_freebsd->device_path);
  command_line = g_strconcat ("eject ", quoted, NULL);
  g_free (quoted);

  /* execute the eject command */
  result = g_spawn_command_line_sync (command_line, NULL, &standard_error, &exit_status, error);
  if (G_LIKELY (result))
    {
      /* check if the command failed */
      if (G_UNLIKELY (exit_status != 0))
        {
          /* drop additional whitespace from the stderr output */
          g_strstrip (standard_error);

          /* generate an error from the standard_error content */
          if (G_LIKELY (g_str_has_prefix (standard_error, "eject: ")))
            {
              /* strip off the "eject: " prefix */
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s", standard_error + 7);
            }
          else
            {
              /* use the error message as-is, not nice, but hey, better than nothing */
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s", standard_error);
            }

          /* and yes, we failed */
          result = FALSE;
        }

      /* release the stderr output */
      g_free (standard_error);
    }

  /* cleanup */
  g_free (command_line);

  /* update volume state if successfull */
  if (G_LIKELY (result))
    thunar_vfs_volume_freebsd_update (volume_freebsd);

  return result;
}



static gboolean
thunar_vfs_volume_freebsd_mount (ThunarVfsVolume *volume,
                                 GtkWidget       *window,
                                 GError         **error)
{
  ThunarVfsVolumeFreeBSD *volume_freebsd = THUNAR_VFS_VOLUME_FREEBSD (volume);
  gboolean                result;
  GString                *message;
  gchar                   mount_point[THUNAR_VFS_PATH_MAXSTRLEN];
  gchar                 **standard_error_parts;
  gchar                  *standard_error;
  gchar                  *command_line;
  gchar                  *quoted;
  gint                    exit_status;
  gint                    n_parts;
  gint                    i;

  /* determine the absolute path to the mount point */
  if (thunar_vfs_path_to_string (volume_freebsd->mount_point, mount_point, sizeof (mount_point), error) < 0)
    return FALSE;

  /* generate the command line for the mount command */
  quoted = g_shell_quote (mount_point);
  command_line = g_strconcat ("mount ", quoted, NULL);
  g_free (quoted);

  /* execute the mount command */
  result = g_spawn_command_line_sync (command_line, NULL, &standard_error, &exit_status, error);
  if (G_LIKELY (result))
    {
      /* check if the command failed */
      if (G_UNLIKELY (exit_status != 0))
        {
          /* drop additional whitespace from the stderr output */
          g_strstrip (standard_error);

          /* the error message will usually look like
           * "cd9660: /dev/cd1: Operation not permitted",
           * so let's apply some voodoo magic here to
           * make it look better.
           */
          standard_error_parts = g_strsplit (standard_error, ":", -1);
          if (G_UNLIKELY (standard_error_parts[0] == NULL))
            {
              /* no useful information, *narf* */
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, _("Unknown error"));
            }
          else
            {
              /* determine the last part of the error */
              for (n_parts = 0; standard_error_parts[n_parts + 1] != NULL; ++n_parts)
                ;

              /* construct a new error message */
              message = g_string_new (g_strstrip (standard_error_parts[n_parts]));

              /* append any additional information in () */
              if (G_LIKELY (n_parts > 0))
                {
                  g_string_append (message, " (");
                  for (i = 0; i < n_parts; ++i)
                    {
                      if (G_UNLIKELY (i > 0))
                        g_string_append (message, ", ");
                      g_string_append (message, g_strstrip (standard_error_parts[i]));
                    }
                  g_string_append (message, ")");
                }

              /* use the generated message for the error */
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s", message->str);
              g_string_free (message, TRUE);
            }

          /* release the standard error parts */
          g_strfreev (standard_error_parts);

          /* and yes, we failed */
          result = FALSE;
        }

      /* release the stderr output */
      g_free (standard_error);
    }

  /* cleanup */
  g_free (command_line);

  /* update volume state if successfull */
  if (G_LIKELY (result))
    thunar_vfs_volume_freebsd_update (volume_freebsd);

  return result;
}



static gboolean
thunar_vfs_volume_freebsd_unmount (ThunarVfsVolume *volume,
                                   GtkWidget       *window,
                                   GError         **error)
{
  ThunarVfsVolumeFreeBSD *volume_freebsd = THUNAR_VFS_VOLUME_FREEBSD (volume);
  gboolean                result;
  gchar                   mount_point[THUNAR_VFS_PATH_MAXSTRLEN];
  gchar                  *standard_error;
  gchar                  *command_line;
  gchar                  *quoted;
  gint                    exit_status;

  /* determine the absolute path to the mount point */
  if (thunar_vfs_path_to_string (volume_freebsd->mount_point, mount_point, sizeof (mount_point), error) < 0)
    return FALSE;

  /* generate the command line for the umount command */
  quoted = g_shell_quote (mount_point);
  command_line = g_strconcat ("umount ", quoted, NULL);
  g_free (quoted);

  /* execute the umount command */
  result = g_spawn_command_line_sync (command_line, NULL, &standard_error, &exit_status, error);
  if (G_LIKELY (result))
    {
      /* check if the command failed */
      if (G_UNLIKELY (exit_status != 0))
        {
          /* drop additional whitespace from the stderr output */
          g_strstrip (standard_error);

          /* generate an error from the standard_error content */
          if (G_LIKELY (g_str_has_prefix (standard_error, "umount: ")))
            {
              /* strip off the "umount: " prefix */
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s", standard_error + 8);
            }
          else
            {
              /* use the error message as-is, not nice, but hey, better than nothing */
              g_set_error (error, G_FILE_ERROR, G_FILE_ERROR_FAILED, "%s", standard_error);
            }


          /* and yes, we failed */
          result = FALSE;
        }

      /* release the stderr output */
      g_free (standard_error);
    }

  /* cleanup */
  g_free (command_line);

  /* update volume state if successfull */
  if (G_LIKELY (result))
    thunar_vfs_volume_freebsd_update (volume_freebsd);

  return result;
}



static gboolean
thunar_vfs_volume_freebsd_update (gpointer user_data)
{
  ThunarVfsVolumeFreeBSD *volume_freebsd = THUNAR_VFS_VOLUME_FREEBSD (user_data);
  ThunarVfsVolumeStatus   status = 0;
  struct ioc_toc_header   ith;
  struct stat             sb;
  gchar                  *label;
  gchar                   buffer[2048];
  int                     fd;

  if (volume_freebsd->kind == THUNAR_VFS_VOLUME_KIND_CDROM)
    {
      /* try to read the table of contents from the CD-ROM,
       * which will only succeed if a disc is present for
       * the drive.
       */
      fd = open (volume_freebsd->device_path, O_RDONLY);
      if (fd >= 0)
        {
          if (ioctl (fd, CDIOREADTOCHEADER, &ith) >= 0)
            {
              status |= THUNAR_VFS_VOLUME_STATUS_PRESENT;

              /* read the label of the disc */
              if (volume_freebsd->label == NULL && (volume_freebsd->status & THUNAR_VFS_VOLUME_STATUS_PRESENT) == 0)
                {
                  /* skip to sector 16 and read it */
                  if (lseek (fd, 16 * 2048, SEEK_SET) >= 0 && read (fd, buffer, sizeof (buffer)) >= 0)
                    {
                      /* offset 40 contains the volume identifier */
                      label = buffer + 40;
                      label[32] = '\0';
                      g_strchomp (label);
                      if (G_LIKELY (*label != '\0'))
                        volume_freebsd->label = g_strdup (label);
                    }
                }
            }

          close (fd);
        }
    }

  /* determine the absolute path to the mount point */
  if (thunar_vfs_path_to_string (volume_freebsd->mount_point, buffer, sizeof (buffer), NULL) > 0)
    {
      /* query the file system information for the mount point */
      if (statfs (buffer, &volume_freebsd->info) >= 0)
        {
          /* if the device is mounted, it means that a medium is present */
          if (exo_str_is_equal (volume_freebsd->info.f_mntfromname, volume_freebsd->device_path))
            status |= THUNAR_VFS_VOLUME_STATUS_MOUNTED | THUNAR_VFS_VOLUME_STATUS_PRESENT;
        }

      /* free the volume label if no disc is present */
      if ((status & THUNAR_VFS_VOLUME_STATUS_PRESENT) == 0)
        {
          g_free (volume_freebsd->label);
          volume_freebsd->label = NULL;
        }

      /* determine the device id if mounted */
      if ((status & THUNAR_VFS_VOLUME_STATUS_MOUNTED) != 0)
        {
          if (stat (buffer, &sb) < 0)
            volume_freebsd->device_id = (ThunarVfsFileDevice) -1;
          else
            volume_freebsd->device_id = sb.st_dev;
        }
    }

  /* update the status if necessary */
  if (status != volume_freebsd->status)
    {
      volume_freebsd->status = status;
      thunar_vfs_volume_changed (THUNAR_VFS_VOLUME (volume_freebsd));
    }

  return TRUE;
}



static ThunarVfsVolumeFreeBSD*
thunar_vfs_volume_freebsd_new (const gchar *device_path,
                               const gchar *mount_path)
{
  ThunarVfsVolumeFreeBSD *volume_freebsd;
  const gchar            *p;

  g_return_val_if_fail (device_path != NULL, NULL);
  g_return_val_if_fail (mount_path != NULL, NULL);

  /* allocate the volume object */
  volume_freebsd = g_object_new (THUNAR_VFS_TYPE_VOLUME_FREEBSD, NULL);
  volume_freebsd->device_path = g_strdup (device_path);
  volume_freebsd->mount_point = thunar_vfs_path_new (mount_path, NULL);

  /* determine the device name */
  for (p = volume_freebsd->device_name = volume_freebsd->device_path; *p != '\0'; ++p)
    if (p[0] == '/' && (p[1] != '/' && p[1] != '\0'))
      volume_freebsd->device_name = p + 1;

  /* determine the kind of the volume */
  p = volume_freebsd->device_name;
  if (p[0] == 'c' && p[1] == 'd' && g_ascii_isdigit (p[2]))
    volume_freebsd->kind = THUNAR_VFS_VOLUME_KIND_CDROM;
  else if (p[0] == 'f' && p[1] == 'd' && g_ascii_isdigit (p[2]))
    volume_freebsd->kind = THUNAR_VFS_VOLUME_KIND_FLOPPY;
  else if ((p[0] == 'a' && p[1] == 'd' && g_ascii_isdigit (p[2]))
        || (p[0] == 'd' && p[1] == 'a' && g_ascii_isdigit (p[2])))
    volume_freebsd->kind = THUNAR_VFS_VOLUME_KIND_HARDDISK;
  else
    volume_freebsd->kind = THUNAR_VFS_VOLUME_KIND_UNKNOWN;

  /* determine up-to-date status */
  thunar_vfs_volume_freebsd_update (volume_freebsd);

  /* start the update timer */
  volume_freebsd->update_timer_id = g_timeout_add (1000, thunar_vfs_volume_freebsd_update, volume_freebsd);

  return volume_freebsd;
}




static void             thunar_vfs_volume_manager_freebsd_class_init         (ThunarVfsVolumeManagerFreeBSDClass *klass);
static void             thunar_vfs_volume_manager_freebsd_manager_init       (ThunarVfsVolumeManagerIface        *iface);
static void             thunar_vfs_volume_manager_freebsd_init               (ThunarVfsVolumeManagerFreeBSD      *manager_freebsd);
static void             thunar_vfs_volume_manager_freebsd_finalize           (GObject                            *object);
static ThunarVfsVolume *thunar_vfs_volume_manager_freebsd_get_volume_by_info (ThunarVfsVolumeManager             *manager,
                                                                              const ThunarVfsInfo                *info);
static GList           *thunar_vfs_volume_manager_freebsd_get_volumes        (ThunarVfsVolumeManager             *manager);



struct _ThunarVfsVolumeManagerFreeBSDClass
{
  GObjectClass __parent__;
};

struct _ThunarVfsVolumeManagerFreeBSD
{
  GObject __parent__;
  GList  *volumes;
};



G_DEFINE_TYPE_WITH_CODE (ThunarVfsVolumeManagerFreeBSD,
                         thunar_vfs_volume_manager_freebsd,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (THUNAR_VFS_TYPE_VOLUME_MANAGER,
                                                thunar_vfs_volume_manager_freebsd_manager_init));



static void
thunar_vfs_volume_manager_freebsd_class_init (ThunarVfsVolumeManagerFreeBSDClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = thunar_vfs_volume_manager_freebsd_finalize;
}



static void
thunar_vfs_volume_manager_freebsd_manager_init (ThunarVfsVolumeManagerIface *iface)
{
  iface->get_volume_by_info = thunar_vfs_volume_manager_freebsd_get_volume_by_info;
  iface->get_volumes = thunar_vfs_volume_manager_freebsd_get_volumes;
}



static void
thunar_vfs_volume_manager_freebsd_init (ThunarVfsVolumeManagerFreeBSD *manager_freebsd)
{
  ThunarVfsVolumeFreeBSD *volume_freebsd;
  struct fstab           *fs;

  /* load the fstab database */
  setfsent ();

  /* process all fstab entries and generate volume objects */
  for (;;)
    {
      /* query the next fstab entry */
      fs = getfsent ();
      if (G_UNLIKELY (fs == NULL))
        break;

      /* we only care for file systems */
      if (!exo_str_is_equal (fs->fs_type, FSTAB_RW)
          && !exo_str_is_equal (fs->fs_type, FSTAB_RQ)
          && !exo_str_is_equal (fs->fs_type, FSTAB_RO))
        continue;

      volume_freebsd = thunar_vfs_volume_freebsd_new (fs->fs_spec, fs->fs_file);
      if (G_LIKELY (volume_freebsd != NULL))
        manager_freebsd->volumes = g_list_append (manager_freebsd->volumes, volume_freebsd);
    }

  /* unload the fstab database */
  endfsent ();
}



static void
thunar_vfs_volume_manager_freebsd_finalize (GObject *object)
{
  ThunarVfsVolumeManagerFreeBSD *manager_freebsd = THUNAR_VFS_VOLUME_MANAGER_FREEBSD (object);
  GList                         *lp;

  g_return_if_fail (THUNAR_VFS_IS_VOLUME_MANAGER (manager_freebsd));

  for (lp = manager_freebsd->volumes; lp != NULL; lp = lp->next)
    g_object_unref (G_OBJECT (lp->data));
  g_list_free (manager_freebsd->volumes);

  G_OBJECT_CLASS (thunar_vfs_volume_manager_freebsd_parent_class)->finalize (object);
}



static ThunarVfsVolume*
thunar_vfs_volume_manager_freebsd_get_volume_by_info (ThunarVfsVolumeManager *manager,
                                                  const ThunarVfsInfo    *info)
{
  ThunarVfsVolumeManagerFreeBSD *manager_freebsd = THUNAR_VFS_VOLUME_MANAGER_FREEBSD (manager);
  ThunarVfsVolumeFreeBSD        *volume_freebsd = NULL;
  GList                         *lp;

  for (lp = manager_freebsd->volumes; lp != NULL; lp = lp->next)
    {
      volume_freebsd = THUNAR_VFS_VOLUME_FREEBSD (lp->data);
      if ((volume_freebsd->status & THUNAR_VFS_VOLUME_STATUS_MOUNTED) != 0 && volume_freebsd->device_id == info->device)
        return THUNAR_VFS_VOLUME (volume_freebsd);
    }

  return NULL;
}



static GList*
thunar_vfs_volume_manager_freebsd_get_volumes (ThunarVfsVolumeManager *manager)
{
  return THUNAR_VFS_VOLUME_MANAGER_FREEBSD (manager)->volumes;
}



