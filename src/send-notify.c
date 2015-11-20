/*
 * This file is part of YAD.
 *
 * YAD is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * YAD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YAD. If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2008-2015, Victor Ananjevsky <ananasik@gmail.com>
 */

#include "yad.h"

void
yad_send_notify ()
{
  static GApplication *app = NULL;
  GNotification *notify;

  if (!app)
    {
      app = g_application_new (NULL, G_APPLICATION_NON_UNIQUE);
      if (options.send_notify_data.appname)
        g_application_set_application_id (app, options.send_notify_data.appname);
    }

  if (!options.extra_data || !options.extra_data[0])
    {
      g_printerr ("yad_send_notify: Title is empty\n");
      return;
    }

  notify = g_notification_new (options.extra_data[0]);
  g_notification_set_priority (notify, options.send_notify_data.prio);

  if (options.extra_data[1])
    g_notification_set_body (notify, options.extra_data[1]);

  if (options.send_notify_data.icons)
    {
      GIcon *icon = g_themed_icon_new_from_names (options.send_notify_data.icons, -1);
      g_notification_set_icon (notify, icon);
      g_object_unref (icon);
    }

  g_application_send_notification (app, NULL, notify);
}
