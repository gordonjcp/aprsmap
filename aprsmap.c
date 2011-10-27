/* vim: set ts=4 sw=4 tw=0: */
/*  aprsmap APRS map display software
	(C) 2010-2011 Gordon JC Pearce MM0YEQ and others
	
	aprsmap.c
	
	This file is part of aprsmap, a simple APRS map viewer using a modular
	and lightweight design
	
	aprsmap is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 2 of the License, or
	any later version.

	aprsmap is distributed in the hope that it will be useful, but
	WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with aprsmap.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <gtk/gtk.h>
#include <aprsmap.h>
#include <mapgui.h>

int load_settings()
{
  GKeyFile *keyfile;
  GKeyFileFlags flags;
  GError *error = NULL;
  gsize length;
  
  /* Create a new GKeyFile object and a bitwise list of flags. */
  keyfile = g_key_file_new ();
  flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
  
  /* Load the GKeyFile from keyfile.conf or return. */
  if (!g_key_file_load_from_file (keyfile, "settings.conf", flags, &error))
  {
    g_error (error->message);
    return -1;
  }
    conf = g_slice_new (APRSMap_Settings);
    
	conf->lat = g_key_file_get_double(keyfile, "Home", "lat", NULL);
	conf->lon = g_key_file_get_double(keyfile, "Home", "lon", NULL);
	conf->zoom = g_key_file_get_integer(keyfile, "Home", "zoom", NULL);
	
	g_key_file_free(keyfile);
	
	set_map_home(conf);
}
 
int main(int argc, char **argv) {

    g_thread_init(NULL);
    gtk_init (&argc, &argv);
    // by the time we get this far, everything else should be running
    // all it remains to do is fire up the main loop
    mainwindow();
    
    load_settings();
    
    gtk_main();
}
