#!/usr/bin/python2

"""
Copyright (C) Hadley Rich 2008 <hads@nice.net.nz>
based on main.c - with thanks to John Stowers

This is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License
as published by the Free Software Foundation; version 2.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
"""

import sys
import os.path
import gtk.gdk
import gobject

import sqlite3, time

gobject.threads_init()
gtk.gdk.threads_init()

#Try static lib first
mydir = os.path.dirname(os.path.abspath(__file__))
libdir = os.path.abspath(os.path.join(mydir, "..", "python", ".libs"))
sys.path.insert(0, libdir)

import osmgpsmap
print "using library: %s (version %s)" % (osmgpsmap.__file__, osmgpsmap.__version__)

assert osmgpsmap.__version__ == "0.7.3"

class DummyMapNoGpsPoint(osmgpsmap.GpsMap):
    def do_draw_gps_point(self, drawable):
        pass
gobject.type_register(DummyMapNoGpsPoint)

class DummyLayer(gobject.GObject, osmgpsmap.GpsMapLayer):
    def __init__(self):
        gobject.GObject.__init__(self)

    def do_draw(self, gpsmap, gdkdrawable):
        pass

    def do_render(self, gpsmap):
        pass

    def do_busy(self):
        return False

    def do_button_press(self, gpsmap, gdkeventbutton):
        return False
gobject.type_register(DummyLayer)

class Database():
    def __init__(self):
        self.conn = sqlite3.connect("./aprs.db")
        self.crs = self.conn.cursor()
    def get_stations(self):
        self.crs.execute("SELECT DISTINCT call, count(call) FROM call_data GROUP BY call;")
        return self.crs.fetchall()
    def get_points(self, station):
        self.crs.execute("SELECT lat,lon, call FROM call_data WHERE call = ?;", (station,))
        return self.crs.fetchall()
        

class UI(gtk.Window):
    def __init__(self):
        gtk.Window.__init__(self, gtk.WINDOW_TOPLEVEL)
        
        db.get_stations()
        self.set_default_size(900, 675)
        self.connect('destroy', lambda x: gtk.main_quit())
        self.set_title('OpenStreetMap GPS Mapper')

        self.vbox = gtk.VBox(False, 0)
        self.add(self.vbox)

        if 0:
            self.osm = DummyMapNoGpsPoint()
        else:
            self.osm = osmgpsmap.GpsMap()
        #self.osm.layer_add(
        #            osmgpsmap.GpsMapOsd(
        #                show_dpad=True,
        #                show_zoom=True))
        self.osm.layer_add(
                    DummyLayer())

        self.osm.connect('button_release_event', self.map_clicked)

        #connect keyboard shortcuts
        self.osm.set_keyboard_shortcut(osmgpsmap.KEY_FULLSCREEN, gtk.gdk.keyval_from_name("F11"))
        self.osm.set_keyboard_shortcut(osmgpsmap.KEY_UP, gtk.gdk.keyval_from_name("Up"))
        self.osm.set_keyboard_shortcut(osmgpsmap.KEY_DOWN, gtk.gdk.keyval_from_name("Down"))
        self.osm.set_keyboard_shortcut(osmgpsmap.KEY_LEFT, gtk.gdk.keyval_from_name("Left"))
        self.osm.set_keyboard_shortcut(osmgpsmap.KEY_RIGHT, gtk.gdk.keyval_from_name("Right"))

        #connect to tooltip
        self.osm.props.has_tooltip = True
        self.osm.connect("query-tooltip", self.on_query_tooltip)

        self.osm.set_center_and_zoom(55.872, -4.291, 16)

        self.latlon_entry = gtk.Entry()

        zoom_in_button = gtk.Button(stock=gtk.STOCK_ZOOM_IN)
        zoom_in_button.connect('clicked', self.zoom_in_clicked)
        zoom_out_button = gtk.Button(stock=gtk.STOCK_ZOOM_OUT)
        zoom_out_button.connect('clicked', self.zoom_out_clicked)
        home_button = gtk.Button(stock=gtk.STOCK_HOME)
        home_button.connect('clicked', self.home_clicked)
        cache_button = gtk.Button('Cache')
        cache_button.connect('clicked', self.cache_clicked)

        hbox2 = gtk.HBox(False, 0)



        # liststore for stations
        lstore = gtk.ListStore(str, int)
        s = db.get_stations()
        for i in s:
            lstore.append([i[0], i[1]])
        treeview = gtk.TreeView(lstore)
        rendererText = gtk.CellRendererText()
        column = gtk.TreeViewColumn("Station", rendererText, text=0)
        column.set_sort_column_id(0)  
        column.set_min_width(100)
        treeview.append_column(column)
        
        column = gtk.TreeViewColumn("Points", rendererText, text=1)
        column.set_sort_column_id(1)  
        column.set_min_width(50)
        treeview.append_column(column)

        treeview.columns_autosize()
        treeview.connect("row-activated", self.on_activated)
                
        sw = gtk.ScrolledWindow()
        sw.set_shadow_type(gtk.SHADOW_ETCHED_IN)
        sw.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)
        sw.add(treeview)
        sw.set_size_request(200,-1)
        
        vbox2 = gtk.VBox(False, 0)
        
        calendar = gtk.Calendar()
        vbox2.pack_start(calendar, False)
        vbox2.pack_start(sw)
        
        
        hbox2.pack_start(vbox2, expand=False)      
        hbox2.pack_start(self.osm)
        self.vbox.pack_start(hbox2)
        

        
        hbox = gtk.HBox(False, 0)
        hbox.pack_start(zoom_in_button)
        hbox.pack_start(zoom_out_button)
        hbox.pack_start(home_button)
        hbox.pack_start(cache_button)

        #add ability to test custom map URIs
        ex = gtk.Expander("<b>Map Repository URI</b>")
        ex.props.use_markup = True
        vb = gtk.VBox()
        self.repouri_entry = gtk.Entry()
        self.repouri_entry.set_text(self.osm.props.repo_uri)
        self.image_format_entry = gtk.Entry()
        self.image_format_entry.set_text(self.osm.props.image_format)

        lbl = gtk.Label(
"""
Enter an repository URL to fetch map tiles from in the box below. Special metacharacters may be included in this url

<i>Metacharacters:</i>
\t#X\tMax X location
\t#Y\tMax Y location
\t#Z\tMap zoom (0 = min zoom, fully zoomed out)
\t#S\tInverse zoom (max-zoom - #Z)
\t#Q\tQuadtree encoded tile (qrts)
\t#W\tQuadtree encoded tile (1234)
\t#U\tEncoding not implemeted
\t#R\tRandom integer, 0-4""")
        lbl.props.xalign = 0
        lbl.props.use_markup = True
        lbl.props.wrap = True

        ex.add(vb)
        vb.pack_start(lbl, False)

        hb = gtk.HBox()
        hb.pack_start(gtk.Label("URI: "), False)
        hb.pack_start(self.repouri_entry, True)
        vb.pack_start(hb, False)

        hb = gtk.HBox()
        hb.pack_start(gtk.Label("Image Format: "), False)
        hb.pack_start(self.image_format_entry, True)
        vb.pack_start(hb, False)

        gobtn = gtk.Button("Load Map URI")
        gobtn.connect("clicked", self.load_map_clicked)
        vb.pack_start(gobtn, False)

        self.show_tooltips = False
        cb = gtk.CheckButton("Show Location in Tooltips")
        cb.props.active = self.show_tooltips
        cb.connect("toggled", self.on_show_tooltips_toggled)
        self.vbox.pack_end(cb, False)

        cb = gtk.CheckButton("Disable Cache")
        cb.props.active = False
        cb.connect("toggled", self.disable_cache_toggled)
        self.vbox.pack_end(cb, False)

        self.vbox.pack_end(ex, False)
        self.vbox.pack_end(self.latlon_entry, False)
        self.vbox.pack_end(hbox, False)

        gobject.timeout_add(500, self.print_tiles)

    def disable_cache_toggled(self, btn):
        if btn.props.active:
            self.osm.props.tile_cache = osmgpsmap.CACHE_DISABLED
        else:
            self.osm.props.tile_cache = osmgpsmap.CACHE_AUTO

    def on_show_tooltips_toggled(self, btn):
        self.show_tooltips = btn.props.active

    def load_map_clicked(self, button):
        uri = self.repouri_entry.get_text()
        format = self.image_format_entry.get_text()
        if uri and format:
            if self.osm:
                #remove old map
                self.vbox.remove(self.osm)
            try:
                self.osm = osmgpsmap.GpsMap(
                    repo_uri=uri,
                    image_format=format
                )
            except Exception, e:
                print "ERROR:", e
                self.osm = osm.GpsMap()

            self.vbox.pack_start(self.osm, True)
            self.osm.connect('button_release_event', self.map_clicked)
            self.osm.show()

    def print_tiles(self):
        if self.osm.props.tiles_queued != 0:
            print self.osm.props.tiles_queued, 'tiles queued'
        return True

    def zoom_in_clicked(self, button):
        self.osm.set_zoom(self.osm.props.zoom + 1)
 
    def zoom_out_clicked(self, button):
        self.osm.set_zoom(self.osm.props.zoom - 1)

    def home_clicked(self, button):
        pass
    def on_activated(self, widget, row, col):
        def plot_line(station, colour):
            points = db.get_points(station)
            try:
                if self.t:
                    self.osm.track_remove(self.t)
            except:
                pass
            self.t = osmgpsmap.GpsMapTrack()
            self.osm.track_add(self.t)
            self.t.props.color.red=0
            self.t.props.color.blue=1
            print "points for %s" % station
            for i in points:
                print i[0], i[1]
                #p = osmgpsmap.point_new_degrees(float(i[0]),float(i[1]))
                #self.t.add_point(p)
        model = widget.get_model()
        plot_line(model[row][0], 0)

    def on_query_tooltip(self, widget, x, y, keyboard_tip, tooltip, data=None):
        if keyboard_tip:
            return False

        if self.show_tooltips:
            p = osmgpsmap.point_new_degrees(0.0, 0.0)
            self.osm.convert_screen_to_geographic(x, y, p)
            lat,lon = p.get_degrees()
            tooltip.set_markup("%+.4f, %+.4f" % p.get_degrees())
            return True

        return False
 
    def cache_clicked(self, button):
        bbox = self.osm.get_bbox()
        self.osm.download_maps(
            *bbox,
            zoom_start=self.osm.props.zoom,
            zoom_end=self.osm.props.max_zoom
        )

    def map_clicked(self, osm, event):
        lat,lon = self.osm.get_event_location(event).get_degrees()
        if event.button == 1:
            self.latlon_entry.set_text(
                'Map Centre: latitude %s longitude %s' % (
                    self.osm.props.latitude,
                    self.osm.props.longitude
                )
            )
        elif event.button == 2:
            self.osm.gps_add(lat, lon, heading=osmgpsmap.INVALID);
        elif event.button == 3:
            pb = gtk.gdk.pixbuf_new_from_file_at_size ("poi.png", 24,24)
            self.osm.image_add(lat,lon,pb)

if __name__ == "__main__":
    
    db = Database()
    u = UI()
    u.show_all()
    if os.name == "nt": gtk.gdk.threads_enter()
    gtk.main()
    if os.name == "nt": gtk.gdk.threads_leave()

# vim: tabstop=8 expandtab shiftwidth=4 softtabstop=4
