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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <locale.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

#ifndef G_OS_WIN32
# include <sys/shm.h>
# include <gdk/gdkx.h>
#endif

#include "yad.h"

YadOptions options;
GtkWidget *dialog = NULL;

YadNTabs *tabs;
gint t_sem;

void print_result (void);

#ifndef G_OS_WIN32
static void
sa_usr1 (gint sig)
{
  if (options.plug != -1)
    print_result ();
  else
  gtk_dialog_response (GTK_DIALOG (dialog), YAD_RESPONSE_OK);
}

static void
sa_usr2 (gint sig)
{
  if (options.plug != -1)
    gtk_main_quit ();
  else
    gtk_dialog_response (GTK_DIALOG (dialog), YAD_RESPONSE_CANCEL);
}
#endif

static void
btn_cb (GtkWidget *b, gchar *cmd)
{
  if (cmd)
    g_spawn_command_line_async (cmd, NULL);
  else
    {
      gint resp = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (b), "resp"));
      gtk_dialog_response (GTK_DIALOG (dialog), resp);
    }
}

static gboolean
timeout_cb (gpointer data)
{
  static guint count = 1;
  GtkWidget *w = (GtkWidget *) data;

  if (options.data.timeout < count)
    {
      gtk_dialog_response (GTK_DIALOG (dialog), YAD_RESPONSE_TIMEOUT);
      return FALSE;
    }

  if (w)
    {
      gdouble percent = ((gdouble) options.data.timeout - count) / (gdouble) options.data.timeout;
      gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (w), percent);
      if (settings.show_remain)
        {
          gchar *lbl = g_strdup_printf (_("%d sec"), options.data.timeout - count);
          gtk_progress_bar_set_text (GTK_PROGRESS_BAR (w), lbl);
          g_free (lbl);
        }
    }

  count++;

  return TRUE;
}

#if !GTK_CHECK_VERSION(3,0,0)
static void
text_size_allocate_cb (GtkWidget * w, GtkAllocation * al, gpointer data)
{
  PangoLayout *pl = gtk_label_get_layout (GTK_LABEL (w));

  if (pango_layout_is_wrapped (pl))
    gtk_widget_set_size_request (w, al->width, -1);
}
#endif

static void
dlg_response_cb (GtkDialog *dlg, gint id, gint *data)
{
  if (options.mode == YAD_MODE_FILE)
    {
      /* show custom confirmation dialog */
      if (!file_confirm_overwrite (dlg))
        {
          g_signal_stop_emission_by_name (dlg, "response");
          return;
        }
    }

  *data = id;
  gtk_main_quit ();
}

static GtkWidget *
create_layout (GtkWidget *dlg)
{
  GtkWidget *image, *text, *mw, *layout, *exp, *box;

  layout = image = text = mw = exp = NULL;

  /* create image */
  if (options.data.dialog_image)
    {
      GdkPixbuf *pb = NULL;

      pb = get_pixbuf (options.data.dialog_image, YAD_BIG_ICON);
      image = gtk_image_new_from_pixbuf (pb);
      if (pb)
        g_object_unref (pb);

      gtk_widget_set_name (image, "yad-dialog-image");
      gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.0);
    }

  /* create text label */
  if (options.data.dialog_text)
    {
      /* for dnd's tooltip we don't need text label */
      if (options.mode != YAD_MODE_DND || !options.dnd_data.tooltip)
        {
          gchar *buf = g_strcompress (options.data.dialog_text);

          text = gtk_label_new (NULL);
          if (!options.data.no_markup)
            gtk_label_set_markup (GTK_LABEL (text), buf);
          else
            gtk_label_set_text (GTK_LABEL (text), buf);
          g_free (buf);

          gtk_widget_set_name (text, "yad-dialog-label");
          gtk_label_set_line_wrap (GTK_LABEL (text), TRUE);
          gtk_label_set_selectable (GTK_LABEL (text), options.data.selectable_labels);
          gtk_label_set_justify (GTK_LABEL (text), options.data.text_align);
          switch (options.data.text_align)
            {
            case GTK_JUSTIFY_LEFT:
            case GTK_JUSTIFY_FILL:
              gtk_misc_set_alignment (GTK_MISC (text), 0.0, 0.5);
              break;
            case GTK_JUSTIFY_CENTER:
              gtk_misc_set_alignment (GTK_MISC (text), 0.5, 0.5);
              break;
            case GTK_JUSTIFY_RIGHT:
              gtk_misc_set_alignment (GTK_MISC (text), 1.0, 0.5);
              break;
            }
#if !GTK_CHECK_VERSION(3,0,0)
          if (!options.data.fixed)
            g_signal_connect (G_OBJECT (text), "size-allocate", G_CALLBACK (text_size_allocate_cb), NULL);
#endif
        }
    }

  /* create main widget */
  switch (options.mode)
    {
    case YAD_MODE_CALENDAR:
      mw = calendar_create_widget (dlg);
      break;
    case YAD_MODE_COLOR:
      mw = color_create_widget (dlg);
      break;
    case YAD_MODE_DND:
      dnd_init (dlg);
      break;
    case YAD_MODE_ENTRY:
      mw = entry_create_widget (dlg);
      break;
    case YAD_MODE_FILE:
      mw = file_create_widget (dlg);
      break;
    case YAD_MODE_FONT:
      mw = font_create_widget (dlg);
      break;
    case YAD_MODE_FORM:
      mw = form_create_widget (dlg);
      break;
#ifdef HAVE_HTML
    case YAD_MODE_HTML:
      mw = html_create_widget (dlg);
      break;
#endif
    case YAD_MODE_ICONS:
      mw = icons_create_widget (dlg);
      break;
    case YAD_MODE_LIST:
      mw = list_create_widget (dlg);
      break;
    case YAD_MODE_MULTI_PROGRESS:
      mw = multi_progress_create_widget (dlg);
      break;
    case YAD_MODE_NOTEBOOK:
      if (options.plug == -1)
        mw = notebook_create_widget (dlg);
      break;
    case YAD_MODE_PANED:
      if (options.plug == -1)
        mw = paned_create_widget (dlg);
      break;
    case YAD_MODE_PICTURE:
      mw = picture_create_widget (dlg);
      break;
    case YAD_MODE_PROGRESS:
      mw = progress_create_widget (dlg);
      break;
    case YAD_MODE_SCALE:
      mw = scale_create_widget (dlg);
      break;
    case YAD_MODE_TEXTINFO:
      mw = text_create_widget (dlg);
      break;
    default: ;
    }

  /* add expander */
  if (mw && options.data.expander)
    {
      exp = gtk_expander_new_with_mnemonic (options.data.expander);
      gtk_expander_set_expanded (GTK_EXPANDER (exp), FALSE);
      gtk_container_add (GTK_CONTAINER (exp), mw);
    }

  /* create layout */
  if (options.data.image_on_top)
    {
#if !GTK_CHECK_VERSION(3,0,0)
      layout = gtk_vbox_new (FALSE, 0);
      box = gtk_hbox_new (FALSE, 0);
#else
      layout = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
      box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#endif
      if (image)
        gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 2);
      if (text)
        gtk_box_pack_start (GTK_BOX (box), text, TRUE, TRUE, 0);

      gtk_box_pack_start (GTK_BOX (layout), box, FALSE, FALSE, 0);
      if (mw)
        gtk_box_pack_start (GTK_BOX (layout), mw, TRUE, TRUE, 0);
    }
  else
    {
#if !GTK_CHECK_VERSION(3,0,0)
      layout = gtk_hbox_new (FALSE, 0);
      box = gtk_vbox_new (FALSE, 0);
#else
      layout = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
      box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#endif
      if (text)
        gtk_box_pack_start (GTK_BOX (box), text, FALSE, FALSE, 0);
      if (mw)
        gtk_box_pack_start (GTK_BOX (box), mw, TRUE, TRUE, 0);

      if (image)
        gtk_box_pack_start (GTK_BOX (layout), image, FALSE, FALSE, 2);
      gtk_box_pack_start (GTK_BOX (layout), box, TRUE, TRUE, 0);
    }

  return layout;
}

static GtkWidget *
create_dialog (void)
{
  GtkWidget *dlg, *vbox, *bbox, *layout;

  /* create dialog window */
  dlg = gtk_dialog_new ();
  if (options.data.splash)
    gtk_window_set_type_hint (GTK_WINDOW (dlg), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
  else
    gtk_window_set_type_hint (GTK_WINDOW (dlg), GDK_WINDOW_TYPE_HINT_NORMAL);
  gtk_window_set_title (GTK_WINDOW (dlg), options.data.dialog_title);
  gtk_widget_set_name (dlg, "yad-dialog-window");

#ifndef  G_OS_WIN32
  /* FIXME: is that very useful !? */
  if (options.parent)
    {
      gdk_window_set_transient_for (gtk_widget_get_window (dlg),
                                    gdk_x11_window_foreign_new_for_display (gdk_display_get_default (),
                                                                            options.parent));
    }
#endif

  if (options.data.no_escape)
    g_signal_connect (G_OBJECT (dlg), "close", G_CALLBACK (g_signal_stop_emission_by_name), "close");

  /* set window icon */
  if (options.data.window_icon)
    {
      if (g_file_test (options.data.window_icon, G_FILE_TEST_EXISTS))
        gtk_window_set_icon_from_file (GTK_WINDOW (dlg), options.data.window_icon, NULL);
      else
        gtk_window_set_icon_name (GTK_WINDOW (dlg), options.data.window_icon);
    }

  /* set window borders */
  if (options.data.borders == -1)
    options.data.borders = (gint) gtk_container_get_border_width (GTK_CONTAINER (dlg));
  gtk_container_set_border_width (GTK_CONTAINER (dlg), (guint) options.data.borders);

  /* set window size and position */
  if (!options.data.geometry && !options.data.maximized && !options.data.fullscreen)
    {
      gtk_window_set_default_size (GTK_WINDOW (dlg), options.data.width, options.data.height);
      if (options.data.center)
        gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_CENTER);
      else if (options.data.mouse)
        gtk_window_set_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
    }

  /* set window behavior */
  if (options.data.sticky)
    gtk_window_stick (GTK_WINDOW (dlg));
  gtk_window_set_keep_above (GTK_WINDOW (dlg), options.data.ontop);
  gtk_window_set_decorated (GTK_WINDOW (dlg), !options.data.undecorated);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dlg), options.data.skip_taskbar);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (dlg), options.data.skip_taskbar);
  gtk_window_set_accept_focus (GTK_WINDOW (dlg), options.data.focus);

  vbox = gtk_dialog_get_content_area (GTK_DIALOG (dlg));
  layout = create_layout (dlg);

  /* create timeout indicator widget */
  if (options.data.timeout)
    {
      GtkWidget *cbox = NULL, *topb = NULL;

      if (G_LIKELY (options.data.to_indicator) && g_ascii_strcasecmp (options.data.to_indicator, "none"))
        {
          topb = gtk_progress_bar_new ();
          gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (topb), 1.0);
          gtk_widget_set_name (topb, "yad-timeout-indicator");
        }

      /* add timeout indicator */
      if (topb)
        {
          if (strcasecmp (options.data.to_indicator, "top") == 0)
            {
#if !GTK_CHECK_VERSION(3,0,0)
              gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (topb), GTK_PROGRESS_LEFT_TO_RIGHT);
              cbox = gtk_vbox_new (FALSE, 0);
#else
              gtk_orientable_set_orientation (GTK_ORIENTABLE (topb), GTK_ORIENTATION_HORIZONTAL);
              cbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#endif
              gtk_box_pack_start (GTK_BOX (cbox), topb, FALSE, FALSE, 2);
              gtk_box_pack_end (GTK_BOX (cbox), layout, TRUE, TRUE, 0);
            }
          else if (strcasecmp (options.data.to_indicator, "bottom") == 0)
            {
#if !GTK_CHECK_VERSION(3,0,0)
              gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (topb), GTK_PROGRESS_LEFT_TO_RIGHT);
              cbox = gtk_vbox_new (FALSE, 0);
#else
              gtk_orientable_set_orientation (GTK_ORIENTABLE (topb), GTK_ORIENTATION_HORIZONTAL);
              cbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#endif
              gtk_box_pack_start (GTK_BOX (cbox), layout, TRUE, TRUE, 0);
              gtk_box_pack_end (GTK_BOX (cbox), topb, FALSE, FALSE, 2);
            }
          else if (strcasecmp (options.data.to_indicator, "left") == 0)
            {
#if !GTK_CHECK_VERSION(3,0,0)
              gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (topb), GTK_PROGRESS_BOTTOM_TO_TOP);
              cbox = gtk_hbox_new (FALSE, 0);
#else
              gtk_orientable_set_orientation (GTK_ORIENTABLE (topb), GTK_ORIENTATION_VERTICAL);
              gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR (topb), TRUE);
              cbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#endif
              gtk_box_pack_start (GTK_BOX (cbox), topb, FALSE, FALSE, 2);
              gtk_box_pack_end (GTK_BOX (cbox), layout, TRUE, TRUE, 0);
            }
          else if (strcasecmp (options.data.to_indicator, "right") == 0)
            {
#if !GTK_CHECK_VERSION(3,0,0)
              gtk_progress_bar_set_orientation (GTK_PROGRESS_BAR (topb), GTK_PROGRESS_BOTTOM_TO_TOP);
              cbox = gtk_hbox_new (FALSE, 0);
#else
              gtk_orientable_set_orientation (GTK_ORIENTABLE (topb), GTK_ORIENTATION_VERTICAL);
              gtk_progress_bar_set_inverted (GTK_PROGRESS_BAR (topb), TRUE);
              cbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#endif
              gtk_box_pack_start (GTK_BOX (cbox), layout, TRUE, TRUE, 0);
              gtk_box_pack_end (GTK_BOX (cbox), topb, FALSE, FALSE, 2);
            }

          if (settings.show_remain)
            {
              gchar *lbl = g_strdup_printf (_("%d sec"), options.data.timeout);
#if GTK_CHECK_VERSION(3,0,0)
              gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (topb), TRUE);
#endif
              gtk_progress_bar_set_text (GTK_PROGRESS_BAR (topb), lbl);
              g_free (lbl);
            }
        }
      else
        {
#if !GTK_CHECK_VERSION(3,0,0)
          cbox = gtk_vbox_new (FALSE, 0);
#else
          cbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
#endif
          gtk_box_pack_start (GTK_BOX (cbox), layout, TRUE, TRUE, 0);
        }

      if (cbox)
        gtk_box_pack_start (GTK_BOX (vbox), cbox, TRUE, TRUE, 0);

      /* set timeout handler */
      g_timeout_add_seconds (1, timeout_cb, topb);
    }
  else
    gtk_box_pack_start (GTK_BOX (vbox), layout, TRUE, TRUE, 0);

  /* get buttons container */
  bbox = gtk_dialog_get_action_area (GTK_DIALOG (dlg));

#ifdef HAVE_HTML
  /* enable no-buttons mode if --browser is specified and sets no custom buttons */
  if (options.mode == YAD_MODE_HTML && options.html_data.browser && !options.data.buttons)
    options.data.no_buttons = TRUE;
#endif

  /* add buttons */
  if (!options.data.no_buttons)
    {
      if (options.data.buttons)
        {
          GSList *tmp = options.data.buttons;
          do
            {
              GtkWidget *btn;
              YadButton *b = (YadButton *) tmp->data;

              btn = gtk_button_new ();
              gtk_container_add (GTK_CONTAINER (btn), get_label (b->name, 2));
              gtk_button_set_alignment (GTK_BUTTON (btn), 0.5, 0.5);
              g_object_set_data (G_OBJECT (btn), "resp", GINT_TO_POINTER (b->response));
              g_signal_connect (G_OBJECT (btn), "clicked", G_CALLBACK (btn_cb), b->cmd);
              gtk_box_pack_start (GTK_BOX (bbox), btn, FALSE, FALSE, 0);

              tmp = tmp->next;
            }
          while (tmp != NULL);
        }
      else
        {
          if (options.mode == YAD_MODE_PROGRESS || options.mode == YAD_MODE_MULTI_PROGRESS ||
              options.mode == YAD_MODE_DND || options.mode == YAD_MODE_PICTURE)
            gtk_dialog_add_buttons (GTK_DIALOG (dlg), GTK_STOCK_CLOSE, YAD_RESPONSE_OK, NULL);
          else
            {
              if (gtk_alternative_dialog_button_order (NULL))
                gtk_dialog_add_buttons (GTK_DIALOG (dlg),
                                        GTK_STOCK_OK, YAD_RESPONSE_OK, GTK_STOCK_CANCEL, YAD_RESPONSE_CANCEL, NULL);
              else
                gtk_dialog_add_buttons (GTK_DIALOG (dlg),
                                        GTK_STOCK_CANCEL, YAD_RESPONSE_CANCEL, GTK_STOCK_OK, YAD_RESPONSE_OK, NULL);
            }
          gtk_dialog_set_default_response (GTK_DIALOG (dlg), YAD_RESPONSE_OK);
        }
      gtk_button_box_set_layout (GTK_BUTTON_BOX (gtk_dialog_get_action_area (GTK_DIALOG (dlg))),
                                 options.data.buttons_layout);
    }

  /* show widgets */
  gtk_widget_show_all (gtk_dialog_get_content_area (GTK_DIALOG (dlg)));
  /* parse geometry, if given. must be after showing widget */
  if (options.data.geometry && !options.data.maximized && !options.data.fullscreen)
    {
      gtk_widget_realize (dlg);
      gtk_window_parse_geometry (GTK_WINDOW (dlg), options.data.geometry);
    }
  gtk_widget_show (dlg);

  if (options.data.no_buttons)
    gtk_widget_hide (bbox);

  /* set maximized or fixed size after showing widget */
  if (options.data.maximized)
    gtk_window_maximize (GTK_WINDOW (dlg));
  else if (options.data.fullscreen)
    gtk_window_fullscreen (GTK_WINDOW (dlg));
  else
    {
      gtk_window_set_resizable (GTK_WINDOW (dlg), !options.data.fixed);
      if (options.data.fixed)
        gtk_widget_set_size_request (dlg, options.data.width, options.data.height);
    }

#ifndef G_OS_WIN32
  /* print xid */
  if (options.print_xid)
    {
      fprintf (stderr, "0x%lX", GDK_WINDOW_XID (gtk_widget_get_window (dlg)));
      fflush (stderr);
    }
#endif

  return dlg;
}

static void
create_plug (void)
{
  GtkWidget *win, *box;

  tabs = get_tabs (options.plug, FALSE);
  while (!tabs)
    {
      usleep (1000);
      tabs = get_tabs (options.plug, FALSE);
    }

  win = gtk_plug_new (0);
  /* set window borders */
  if (options.data.borders == -1)
    options.data.borders = (gint) gtk_container_get_border_width (GTK_CONTAINER (win));
  gtk_container_set_border_width (GTK_CONTAINER (win), (guint) options.data.borders);

  box = create_layout (win);
  if (box)
    gtk_container_add (GTK_CONTAINER (win), box);

  gtk_widget_show_all (win);

  /* add plug data */
  tabs[options.tabnum].pid = getpid ();
  tabs[options.tabnum].xid = gtk_plug_get_id (GTK_PLUG (win));
  /* FIXME: may be a race here */
  tabs[0].xid++;
  shmdt (tabs);
}

void
print_result (void)
{
  switch (options.mode)
    {
    case YAD_MODE_CALENDAR:
      calendar_print_result ();
      break;
    case YAD_MODE_COLOR:
      color_print_result ();
      break;
    case YAD_MODE_ENTRY:
      entry_print_result ();
      break;
    case YAD_MODE_FILE:
      file_print_result ();
      break;
    case YAD_MODE_FONT:
      font_print_result ();
      break;
    case YAD_MODE_FORM:
      form_print_result ();
      break;
    case YAD_MODE_LIST:
      list_print_result ();
      break;
    case YAD_MODE_NOTEBOOK:
      notebook_print_result ();
      break;
    case YAD_MODE_PANED:
      paned_print_result ();
      break;
    case YAD_MODE_SCALE:
      scale_print_result ();
      break;
    case YAD_MODE_TEXTINFO:
      text_print_result ();
      break;
    default:;
    }
}

gint
main (gint argc, gchar ** argv)
{
  GOptionContext *ctx;
  GError *err = NULL;
  gint w, h;
  gchar *str;
  gint ret = YAD_RESPONSE_ESC;

  setlocale (LC_ALL, "");

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

#if !GLIB_CHECK_VERSION(2,36,0)
  g_type_init ();
#endif
  read_settings ();

  gtk_init (&argc, &argv);
  g_set_application_name ("YAD");
  yad_options_init ();

  ctx = yad_create_context ();
  /* parse YAD_OPTIONS */
  if (g_getenv ("YAD_OPTIONS"))
    {
      gchar *cmd, **args = NULL;
      gint cnt;

      cmd = g_strdup_printf ("yad %s", g_getenv ("YAD_OPTIONS"));

      if (g_shell_parse_argv (cmd, &cnt, &args, &err))
        {
          g_option_context_parse (ctx, &cnt, &args, &err);
          if (err)
            {
              g_printerr (_("Unable to parse YAD_OPTIONS: %s\n"), err->message);
              g_error_free (err);
              err = NULL;
            }
        }
      else
        {
          g_printerr (_("Unable to parse YAD_OPTIONS: %s\n"), err->message);
          g_error_free (err);
          err = NULL;
        }

      g_free (cmd);
    }
  /* parse command line */
  g_option_context_parse (ctx, &argc, &argv, &err);
  if (err)
    {
      g_printerr (_("Unable to parse command line: %s\n"), err->message);
      return -1;
    }
  yad_set_mode ();

  /* parse custom gtkrc */
  if (options.gtkrc_file)
    gtk_rc_parse (options.gtkrc_file);

  /* set default icons and icon theme */
  if (options.data.icon_theme)
    {
      settings.icon_theme = gtk_icon_theme_new ();
      gtk_icon_theme_set_custom_theme (settings.icon_theme, options.data.icon_theme);
    }
  else
    settings.icon_theme = gtk_icon_theme_get_default ();
  gtk_icon_size_lookup (GTK_ICON_SIZE_DIALOG, &w, &h);
  settings.big_fallback_image =
    gtk_icon_theme_load_icon (settings.icon_theme, "yad", MIN (w, h), GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);
  gtk_icon_size_lookup (GTK_ICON_SIZE_MENU, &w, &h);
  settings.small_fallback_image =
    gtk_icon_theme_load_icon (settings.icon_theme, "yad", MIN (w, h), GTK_ICON_LOOKUP_GENERIC_FALLBACK, NULL);

  /* correct separators */
  str = g_strcompress (options.common_data.separator);
  options.common_data.separator = str;
  str = g_strcompress (options.common_data.item_separator);
  options.common_data.item_separator = str;

  /* loads an extra arguments, if specified */
  if (options.rest_file)
    {
      GIOChannel *ioc;
      gchar *buf;
      guint len, line = 0;

      g_strfreev (options.extra_data);
      options.extra_data = NULL;

      ioc = g_io_channel_new_file (options.rest_file, "r", NULL);
      while (TRUE)
        {
          gint status = g_io_channel_read_line (ioc, &buf, NULL, NULL, NULL);

          if (status != G_IO_STATUS_NORMAL)
            break;

          /* remove \n at the end of string */
          len = strlen (buf);
          if (buf[len - 1] == '\n')
            buf[len - 1] = '\0';

          /* add line to arguments array */
          options.extra_data = g_realloc (options.extra_data, (line + 2) * sizeof (gchar *));
          options.extra_data[line] = g_strcompress (buf);
          options.extra_data[line + 1] = NULL;

          g_free (buf);
          line++;
        }
      g_io_channel_shutdown (ioc, FALSE, NULL);
    }

#ifndef G_OS_WIN32
  /* add YAD_PID variable */
  str = g_strdup_printf ("%d", getpid ());
  g_setenv ("YAD_PID", str, TRUE);
  /* set signal handlers */
  signal (SIGUSR1, sa_usr1);
  signal (SIGUSR2, sa_usr2);
#endif

  /* plug mode */
  if (options.plug != -1)
    {
      create_plug ();
      gtk_main ();
      shmdt (tabs);
      return ret;
    }

  switch (options.mode)
    {
    case YAD_MODE_ABOUT:
      ret = yad_about ();
      break;

    case YAD_MODE_VERSION:
      g_print ("%s (GTK+ %d.%d.%d)\n", VERSION, gtk_major_version, gtk_minor_version, gtk_micro_version);
      break;

    case YAD_MODE_NOTIFICATION:
      ret = yad_notification_run ();
      break;

    case YAD_MODE_PRINT:
      ret = yad_print_run ();
      break;

    default:
      dialog = create_dialog ();
      g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (dlg_response_cb), &ret);
      gtk_widget_show_all (dialog);

#ifndef G_OS_WIN32
      /* add YAD_XID variable */
      str = g_strdup_printf ("0x%X", (guint) GDK_WINDOW_XID (gtk_widget_get_window (dialog)));
      g_setenv ("YAD_XID", str, TRUE);
#endif

      /* make some specific init actions */
      if (options.mode == YAD_MODE_NOTEBOOK)
        notebook_swallow_childs ();
      else if (options.mode == YAD_MODE_PANED)
        paned_swallow_childs ();
      else if (options.mode == YAD_MODE_PICTURE)
        {
          if (options.picture_data.size == YAD_PICTURE_FIT)
            picture_fit_to_window ();
        }

      /* run main loop */
      gtk_main ();

      /* print results */
      if (ret != YAD_RESPONSE_TIMEOUT && ret != YAD_RESPONSE_ESC)
        {
          if (options.data.always_print)
            print_result ();
          else
            {
              /* standard OK button pressed */
              if (ret == YAD_RESPONSE_OK && options.data.buttons == NULL)
                print_result ();
              /* custom even button pressed */
              else if (!(ret & 1))
                print_result ();
            }
        }
#ifndef G_OS_WIN32
      if (options.mode == YAD_MODE_NOTEBOOK)
        notebook_close_childs ();
      else if (options.mode == YAD_MODE_PANED)
        paned_close_childs ();
      /* autokill option for progress dialog */
      if (!options.kill_parent)
        {
          if ((options.mode == YAD_MODE_PROGRESS || options.mode == YAD_MODE_MULTI_PROGRESS) &&
              options.progress_data.autokill && ret != YAD_RESPONSE_OK)
            kill (getppid (), SIGHUP);
        }
#endif
    }

#ifndef G_OS_WIN32
  /* NSIG defined in signal.h */
  if (options.kill_parent > 0 && options.kill_parent < NSIG)
    kill (getppid (), options.kill_parent);
#endif

  return ret;
}
