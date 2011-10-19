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

#ifndef _MAPGUI_H
#define _MAPGUI_H

#include <gtk/gtk.h>

GtkBuilder *builder;    // FIXME - global? Do we need this other places?
void mainwindow();
void set_map_home(APRSMap_Settings *conf);
#endif
