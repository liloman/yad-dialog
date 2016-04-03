#include <glib.h>
#include <gtkspell/gtkspell.h>

int main (int argc, char *argv[])
{
  GList *lng;
  
  for (lng = gtk_spell_checker_get_language_list (); lng; lng = lng->next)
    g_print ("%s\n", lng->data);

  return 0;
}
