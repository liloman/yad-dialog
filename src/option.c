
#include <config.h>

#include "yad.h"

static gboolean add_button (const gchar *, const gchar *, gpointer, GError **);
static gboolean add_column (const gchar *, const gchar *, gpointer, GError **);
static gboolean add_field (const gchar *, const gchar *, gpointer, GError **);

static gboolean about_mode = FALSE;
static gboolean version_mode = FALSE;
static gboolean calendar_mode = FALSE;
static gboolean color_mode = FALSE;
static gboolean entry_mode = FALSE;
static gboolean file_mode = FALSE;
static gboolean form_mode = FALSE;
static gboolean list_mode = FALSE;
static gboolean notification_mode = FALSE;
static gboolean progress_mode = FALSE;
static gboolean scale_mode = FALSE;
static gboolean text_mode = FALSE;

static GOptionEntry general_options[] = {
  { "title", 0,
    0,
    G_OPTION_ARG_STRING,
    &options.data.dialog_title,
    N_("Set the dialog title"),
    N_("TITLE") },
  { "window-icon", 0,
    0,
    G_OPTION_ARG_FILENAME,
    &options.data.window_icon,
    N_("Set the window icon"),
    N_("ICONPATH") },
  { "width", 0,
    0,
    G_OPTION_ARG_INT,
    &options.data.width,
    N_("Set the width"),
    N_("WIDTH") },
  { "height", 0,
    0,
    G_OPTION_ARG_INT,
    &options.data.height,
    N_("Set the height"),
    N_("HEIGHT") },
  { "timeout", 0,
    0,
    G_OPTION_ARG_INT,
    &options.data.timeout,
    N_("Set dialog timeout in seconds"),
    N_("TIMEOUT") },
  { "text", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &options.data.dialog_text,
    N_("Set the dialog text"),
    N_("TEXT") },
  { "image", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_FILENAME,
    &options.data.dialog_image,
    N_("Set the dialog image"),
    N_("IMAGE") },
  { "no-wrap", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &options.data.no_wrap,
    N_("Do not enable text wrapping"),
    NULL },
  { "button", 0,
    0,
    G_OPTION_ARG_CALLBACK,
    add_button,
    N_("Add dialog button (may be used multiple times)"),
    N_("NAME:ID") },
  { "dialog-sep", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.data.dialog_sep,
    N_("Add separator between dialog and buttons"),
    NULL },
  { NULL }
};

static GOptionEntry calendar_options[] = {
  { "calendar", 0,
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &calendar_mode,
    N_("Display calendar dialog"),
    NULL },
  { "day", 0,
    0,
    G_OPTION_ARG_INT,
    &options.calendar_data.day,
    N_("Set the calendar day"),
    N_("DAY") },
  { "month", 0,
    0,
    G_OPTION_ARG_INT,
    &options.calendar_data.month,
    N_("Set the calendar month"),
    N_("MONTH") },
  { "year", 0,
    0,
    G_OPTION_ARG_INT,
    &options.calendar_data.year,
    N_("Set the calendar year"),
    N_("YEAR") },
  { "date-format", 0,
    0,
    G_OPTION_ARG_STRING,
    &options.calendar_data.date_format,
    N_("Set the format for the returned date"),
    N_("PATTERN") },
  {
    NULL}
};

static GOptionEntry color_options[] = {
  { "color", 0,
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &color_mode,
    N_("Display color selection dialog"),
    NULL },
  { "value", 0,
    0,
    G_OPTION_ARG_STRING,
    &options.color_data.init_color,
    N_("Set initial color value"),
    N_("COLOR") },
  { "extra", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.color_data.extra,
    N_("Use #rrrrggggbbbb format instead of #rrggbb"),
    NULL },
  { NULL }
};

static GOptionEntry entry_options[] = {
  { "entry", 0,
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &entry_mode,
    N_("Display text entry or combo-box dialog"),
    NULL },
  { "entry-label", 0,
    0,
    G_OPTION_ARG_STRING,
    &options.entry_data.entry_label,
    N_("Set the entry label"),
    N_("TEXT") },
  { "entry-text", 0,
    0,
    G_OPTION_ARG_STRING,
    &options.entry_data.entry_text,
    N_("Set the entry text"),
    N_("TEXT") },
  { "hide-text", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.entry_data.hide_text,
    N_("Hide the entry text"),
    N_("TEXT") },
  { "completion", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.entry_data.completion,
    N_("Use completion instead of combo-box"),
    NULL },
  { "editable", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &options.common_data.editable,
    N_("Allow changes to text in combo-box"),
    NULL },
  { NULL }
};

static GOptionEntry file_selection_options[] = {
  { "file-selection", 0,
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &file_mode,
    N_("Display file selection dialog"),
    NULL },
  { "filename", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_FILENAME,
    &options.common_data.uri,
    N_("Set the filename"),
    N_("FILENAME") },
  { "multiple", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &options.common_data.multi,
    N_("Allow multiple files to be selected"),
    NULL },
  { "directory", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.file_data.directory,
    N_("Activate directory-only selection"),
    NULL },
  { "save", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.file_data.save,
    N_("Activate save mode"),
    NULL },
  { "separator", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &options.common_data.separator,
    N_("Set output separator character"),
    N_("SEPARATOR") },
  { "confirm-overwrite", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.file_data.confirm_overwrite,
    N_("Confirm file selection if filename already exists"),
    NULL },
  { "file-filter", 0,
    0,
    G_OPTION_ARG_STRING_ARRAY,
    &options.file_data.filter,
    N_("Sets a filename filter"),
    N_("NAME | PATTERN1 PATTERN2 ...") },
  { NULL}
};

static GOptionEntry form_options[] = {
  { "form", 0,
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &form_mode,
    N_("Display form dialog"),
    NULL },
  { "field", 0,
    0,
    G_OPTION_ARG_CALLBACK,
    add_field,
    N_("Add field to form"),
    N_("LABEL") },
  { "separator", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &options.common_data.separator,
    N_("Set output separator character"),
    N_("SEPARATOR") },
  { NULL}
};

static GOptionEntry list_options[] = {
  { "list", 0,
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &list_mode,
    N_("Display list dialog"),
    NULL },
  { "column", 0,
    0,
    G_OPTION_ARG_CALLBACK,
    add_column,
    N_("Set the column header"),
    N_("COLUMN") },
  { "checklist", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.list_data.checkbox,
    N_("Use check boxes for first column"),
    NULL },
  { "radiolist", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.list_data.checkbox,
    N_("Alias to checklist (deprecated)"),
    NULL },
  { "separator",
    0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_STRING,
    &options.common_data.separator,
    N_("Set output separator character"),
    N_("SEPARATOR") },
  { "multiple", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &options.common_data.multi,
    N_("Allow multiple rows to be selected"),
    NULL },
  { "editable", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &options.common_data.editable,
    N_("Allow changes to text"),
    NULL },
  { "print-all", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.list_data.print_all,
    N_("Print all data from list"),
    NULL },
  { "print-column", 0,
    0,
    G_OPTION_ARG_INT,
    &options.list_data.print_column,
    N_("Print a specific column (Default is 1. 0 can be used to print all columns)"),
    N_("NUMBER") },
  { "hide-column", 0,
    0,
    G_OPTION_ARG_INT,
    &options.list_data.hide_column,
    N_("Hide a specific column"),
    N_("NUMBER") },
  { NULL }
};

static GOptionEntry notification_options[] = {
  { "notification", 0,
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &notification_mode,
    N_("Display notification"),
    NULL},
  { "command", 0,
    0,
    G_OPTION_ARG_STRING,
    &options.notification_data.command,
    N_("Listen for commands on stdin"),
    NULL },
  { "listen", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.notification_data.listen,
    N_("Listen for commands on stdin"),
    NULL },
  { NULL }
};

static GOptionEntry progress_options[] = {
  { "progress", 0,
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &progress_mode,
    N_("Display progress indication dialog"),
    NULL },
  { "progress-text", 0,
    0,
    G_OPTION_ARG_STRING,
    &options.progress_data.progress_text,
    N_("Set progress text"),
    N_("TEXT") },
  { "percentage", 0,
    0,
    G_OPTION_ARG_INT,
    &options.progress_data.percentage,
    N_("Set initial percentage"),
    N_("PERCENTAGE") },
  { "pulsate", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.progress_data.pulsate,
    N_("Pulsate progress bar"),
    NULL },
  { "auto-close", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.progress_data.autoclose,
    /* xgettext: no-c-format */
    N_("Dismiss the dialog when 100% has been reached"),
    NULL },
  { "auto-kill", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.progress_data.autokill,
    N_("Kill parent process if cancel button is pressed"),
    NULL },
  { NULL }
};

static GOptionEntry scale_options[] = {
  { "scale", 0,
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &scale_mode,
    N_("Display scale dialog"),
    NULL },
  { "value", 0,
    0,
    G_OPTION_ARG_INT,
    &options.scale_data.value,
    N_("Set initial value"),
    N_("VALUE") },
  { "min-value", 0,
    0,
    G_OPTION_ARG_INT,
    &options.scale_data.min_value,
    N_("Set minimum value"),
    N_("VALUE") },
  { "max-value", 0,
    0,
    G_OPTION_ARG_INT,
    &options.scale_data.max_value,
    N_("Set maximum value"),
    N_("VALUE") },
  { "step", 0,
    0,
    G_OPTION_ARG_INT,
    &options.scale_data.step,
    N_("Set step size"),
    N_("VALUE") },
  { "print-partial", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.scale_data.print_partial,
    N_("Print partial values"),
    NULL },
  { "hide-value", 0,
    0,
    G_OPTION_ARG_NONE,
    &options.scale_data.hide_value,
    N_("Hide value"),
    NULL },
  { NULL }
};

static GOptionEntry text_options[] = {
  { "text-info", 0,
    G_OPTION_FLAG_IN_MAIN,
    G_OPTION_ARG_NONE,
    &text_mode,
    N_("Display text information dialog"),
    NULL },
  { "font", 0,
    0,
    G_OPTION_ARG_STRING,
    &options.text_data.font,
    N_("Use specified font"),
    N_("FONTNAME") },
  { "filename", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_FILENAME,
    &options.common_data.uri,
    N_("Open file"),
    N_("FILENAME") },
  { "editable", 0,
    G_OPTION_FLAG_NOALIAS,
    G_OPTION_ARG_NONE,
    &options.common_data.editable,
    N_("Allow changes to text"),
    NULL },
  { NULL }
};

static GOptionEntry misc_options[] = {
  { "about", 0,
    0,
    G_OPTION_ARG_NONE,
    &about_mode,
    N_("Show about dialog"),
    NULL },
  { "version", 0,
    0,
    G_OPTION_ARG_NONE,
    &version_mode,
    N_("Print version"),
    NULL },
  { NULL }
};

static GOptionEntry rest_options[] = {
  { G_OPTION_REMAINING, 0, 
    0, 
    G_OPTION_ARG_STRING_ARRAY, 
    &options.extra_data,
    NULL, NULL },
  { NULL }
};

static gboolean
add_button (const gchar *option_name,
	    const gchar *value,
	    gpointer data, GError **err)
{
  YadButton *btn;
  gchar **bstr = g_strsplit (value, ":", 2);
  
  btn = g_new0 (YadButton, 1);
  btn->name = g_strdup (bstr[0]);
  btn->response = g_ascii_strtoll (bstr[1], NULL, 10);
  options.data.buttons = g_slist_append (options.data.buttons, btn);
  
  g_strfreev (bstr);

  return TRUE;
}

static gboolean
add_column (const gchar *option_name,
	    const gchar *value,
	    gpointer data, GError **err)
{
  gchar *name = g_strdup (value);

  options.list_data.columns = 
    g_slist_append (options.list_data.columns, name);
  return TRUE;
}

static gboolean
add_field (const gchar *option_name,
	   const gchar *value,
	   gpointer data, GError **err)
{
  gchar *name = g_strdup (value);

  options.form_data.fields =
    g_slist_append (options.form_data.fields, name);
  return TRUE;
}

void
yad_set_mode (void)
{
  if (calendar_mode)
    options.mode = MODE_CALENDAR;
  if (color_mode)
    options.mode = MODE_COLOR;
  else if (entry_mode)
    options.mode = MODE_ENTRY;
  else if (file_mode)
    options.mode = MODE_FILE;
  else if (form_mode)
    options.mode = MODE_FORM;
  else if (list_mode)
    options.mode = MODE_LIST;
  else if (notification_mode)
    options.mode = MODE_NOTIFICATION;
  else if (progress_mode)
    options.mode = MODE_PROGRESS;
  else if (scale_mode)
    options.mode = MODE_SCALE;
  else if (text_mode)
    options.mode = MODE_TEXTINFO;
  else if (about_mode)
    options.mode = MODE_ABOUT;
  else if (version_mode)
    options.mode = MODE_VERSION;
}

void
yad_options_init (void)
{
  /* Set default mode */
  options.mode = MODE_MESSAGE;
  options.extra_data = NULL;

  /* Initialize general data */
  options.data.dialog_title = NULL;
  options.data.window_icon = "yad";
  options.data.width = settings.width;
  options.data.height = settings.height;
  options.data.dialog_text = NULL;
  options.data.dialog_image = NULL;
  options.data.no_wrap = FALSE;
  options.data.timeout = settings.timeout;
  options.data.buttons = NULL;
  options.data.dialog_sep = settings.dlg_sep;

  /* Initialize window options */
  options.data.sticky = FALSE;
  options.data.fixed = FALSE;
  options.data.ontop = FALSE;
  options.data.center = FALSE;
  options.data.decorated = FALSE;

  /* Initialize common data */
  options.common_data.uri = NULL;
  options.common_data.separator = settings.sep;
  options.common_data.multi = FALSE;
  options.common_data.editable = FALSE;  

  /* Initialize calendar data */
  options.calendar_data.date_format = NULL;
  options.calendar_data.day = -1;
  options.calendar_data.month = -1;
  options.calendar_data.year = -1;

  /* I(nitialize color data */
  options.color_data.init_color = NULL;
  options.color_data.extra = FALSE;

  /* Initialize entry data */
  options.entry_data.entry_text = NULL;
  options.entry_data.entry_label = NULL;
  options.entry_data.hide_text = FALSE;
  options.entry_data.completion = FALSE;

  /* Initialize file data */
  options.file_data.directory = FALSE;
  options.file_data.save = FALSE;
  options.file_data.confirm_overwrite = FALSE;
  options.file_data.filter = NULL;

  /* Initialize form data */
  options.form_data.fields = NULL;

  /* Initialize list data */
  options.list_data.columns = NULL;
  options.list_data.checkbox = FALSE;
  options.list_data.print_all = FALSE;
  options.list_data.print_column = 1;
  options.list_data.hide_column = 0;

  /* Initialize notification data */
  options.notification_data.command = NULL;
  options.notification_data.listen = FALSE;

  /* Initialize progress data */
  options.progress_data.progress_text = NULL;
  options.progress_data.percentage = 0;
  options.progress_data.pulsate = FALSE;
  options.progress_data.autoclose = FALSE;
  options.progress_data.autokill = FALSE;

  /* Initialize scale data */
  options.scale_data.value = 0;
  options.scale_data.min_value = 0;
  options.scale_data.max_value = 100;
  options.scale_data.step = 1;
  options.scale_data.print_partial = FALSE;
  options.scale_data.hide_value = FALSE;

  /* Initialize text data */
  options.text_data.font = NULL;
}

GOptionContext *
yad_create_context (void)
{
  GOptionContext *tmp_ctx;
  GOptionGroup *a_group;

  tmp_ctx = g_option_context_new (_("Yet another dialoging program"));
  g_option_context_add_main_entries (tmp_ctx, rest_options, GETTEXT_PACKAGE);
 
  /* Adds general option entries */
  a_group = g_option_group_new ("general", _("General options"),
				_("Show general options"), NULL, NULL);
  g_option_group_add_entries (a_group, general_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds calendar option entries */
  a_group = g_option_group_new ("calendar", _("Calendar options"),
				_("Show calendar options"), NULL, NULL);
  g_option_group_add_entries (a_group, calendar_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds calendar option entries */
  a_group = g_option_group_new ("color", _("Color options"),
				_("Show color options"), NULL, NULL);
  g_option_group_add_entries (a_group, color_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds entry option entries */
  a_group = g_option_group_new ("entry", _("Text entry options"),
				_("Show text entry options"), NULL, NULL);
  g_option_group_add_entries (a_group, entry_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds file selection option entries */
  a_group = g_option_group_new ("file", _("File selection options"),
				_("Show file selection options"), NULL, NULL);
  g_option_group_add_entries (a_group, file_selection_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Add form option entries */
  a_group = g_option_group_new ("form", _("Form options"),
				_("Show form options"), NULL, NULL);
  g_option_group_add_entries (a_group, form_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds list option entries */
  a_group = g_option_group_new ("list", _("List options"),
				_("Show list options"), NULL, NULL);
  g_option_group_add_entries (a_group, list_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds notification option entries */
  a_group = g_option_group_new ("notification", _("Notification icon options"),
				_("Show notification icon options"), NULL, NULL);
  g_option_group_add_entries (a_group, notification_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds progress option entries */
  a_group = g_option_group_new ("progress", _("Progress options"),
				_("Show progress options"), NULL, NULL);
  g_option_group_add_entries (a_group, progress_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds scale option entries */
  a_group = g_option_group_new ("scale", _("Scale options"),
				_("Show scale options"), NULL, NULL);
  g_option_group_add_entries (a_group, scale_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds text option entries */
  a_group = g_option_group_new ("text", _("Text information options"),
				_("Show text information options"), NULL, NULL);
  g_option_group_add_entries (a_group, text_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds miscellaneous option entries */
  a_group = g_option_group_new ("misc", _("Miscellaneous options"),
				_("Show miscellaneous options"), NULL, NULL);
  g_option_group_add_entries (a_group, misc_options);
  g_option_group_set_translation_domain (a_group, GETTEXT_PACKAGE);
  g_option_context_add_group (tmp_ctx, a_group);

  /* Adds gtk option entries */
  a_group = gtk_get_option_group (TRUE);
  g_option_context_add_group (tmp_ctx, a_group);

  g_option_context_set_help_enabled (tmp_ctx, TRUE);
  g_option_context_set_ignore_unknown_options (tmp_ctx, FALSE);

  return tmp_ctx;
}
