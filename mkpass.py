#!/usr/bin/env python3
# -*- Mode: python; tab-width: 4; indent-tabs-mode: nil -*-

################################################################################
#
# Usage: mkpass.py [-n]
#
# mkpass.py is a simple password generator script.
# The script asks for a master password blindly and an additional string
# that may be a service name, a URL or whatever else, and generates an 8
# character long password which is a base64 encoded sha1 hash of the two master
# and site specific passwords put together.
#
# Kirill Gorelov <kgorelov@gmail.com>
################################################################################

import os
import sys
import hashlib
import base64
import optparse
import sqlite3
import readline

################################################################################

# Generates base64 encoded SHA1 out of master and slave passwords
def mkpass(master, slave, len=8):
    sha = hashlib.sha1()
    thestring = master + slave
    sha.update(thestring.encode('utf-8'))
    hash = sha.digest()
    return base64.b64encode(hash)[:len]

################################################################################

class DBException:
    def __init__(self, what):
        self.what = what
    def __str__(self):
        return repr(self.what)

################################################################################

class DBConverter:
    def __init__(self):
        pass
    def convert_v0_to_v1(self, ):
        pass

################################################################################

class ConfigDB:
    # The following represents the class version.
    # The program will abort and prompt a user to update the program
    # if this version is less than the database version.
    # The program will update database structure to the current version
    # if this version is greater than the database version.
    VERSION = 1
    def __init__(self):
        self.dbfile = os.path.expanduser('~') + "/.mkpass.db"
        # Connect to the database
        self.conn = sqlite3.connect(self.dbfile)
        cur = self.conn.cursor();

        # Check database version
        db_version = self.get_db_version(cur) 
        if self.get_db_version(cur) != self.VERSION:
            raise DBException("Database version mismatch: %d" % db_version)

        # Create db if necessary
        self.create_db(cur);

        # Load properties
        cur.execute("select name, length from snames")
        self.names = dict([(i[0], i[1]) for i in cur.fetchall()])
        # print self.get_db_version() # XXX DEBUG RMME

    def create_db(self, cursor = None):
        cur = cursor if cursor is not None else self.conn.cursor()
        # Create tables when needed
        cur.execute("create table if not exists snames (name TEXT primary key, length INTEGER)")
        cur.execute("create table if not exists params (name TEXT primary key, value TEXT)")

    def get_names(self):
        return self.names.keys()

    def get_len(self, name):
        if name in self.names:
            return self.names[name]
        return None

    def save_name(self, name, length):
        self.names[name] = length
        cur = self.conn.cursor()
        cur.execute("replace into snames values (?, ?)", (name, length))
        self.conn.commit()

    def get_db_version(self, cursor = None):
        cur = cursor if cursor is not None else self.conn.cursor()
        cur.execute("select value from params where name = 'dbversion'")
        strver = cur.fetchone()
        ver = 0
        if strver is not None:
            ver = int(strver[0])
        return ver

    def set_db_version(self, version):
        cur = self.conn.cursor()
        cur.execute("replace into params values(?, ?) name = 'dbversion'",
                    ('dbversion', version))


    def upgrade_db(self):
        # XXX 2DO:
        # Read database version
        # copy original file to backup. (.mkpass.db.backup.v0)
        # Call updaters chain
        pass

################################################################################

class MakePassTUI:
    def __init__(self, pwfun, options, cfg):
        self.pwfun = pwfun
        self.opts = options
        self.cfg = cfg

    def complete(text, state):
        volcab = ['dog','cat','rabbit','bird','slug','snail']
        # volcab = self.cfg.get_names()
        print("volcab = {}".format(volcab))
        results = [x for x in volcab if x.startswith(text)] + [None]
        return results[state]

    def run(self):
        # Prompt for the master password and service specific info
        try:
            master = getpass.getpass("Master password: ")
            if self.opts.newpwd \
                and master != getpass.getpass("ReEnter Master password: "):
                print("Error: Passwords mismatch!")
                return 1

            readline.set_completer(self.complete)
            readline.parse_and_bind("tab: complete")
            print("XXX autocomplete in action")
            service = input("Service: ")
            # Print out the password we've just generated
            savedlen = self.cfg.get_len(service)
            if savedlen is not None:
                self.opts.len = savedlen
            # print("self.opts.len = {}, savedlen = {}".format(
            #     self.opts.len, savedlen))
            print("Password: %s" % self.pwfun(master, service, self.opts.len))
            self.cfg.save_name(service, self.opts.len)
        except KeyboardInterrupt as e:
            return 1
        except EOFError as e:
            return 1

################################################################################

COL_TEXT = 0

class MakePassGUI:
    def copy(self, me, me2, se):
        if self.opts.newpwd and me.get_text() != me2.get_text():
            self.errmsg("Passwords mismatch!")
        else:
            master = me.get_text()
            service = se.get_text()
            text = self.pwfun(master, service, self.opts.len)
            print(text)
            self.clip.set_text(text.decode('utf-8'), len=-1)
            self.cfg.save_name(service, self.opts.len)
            self.set_words(self.cfg.get_names())

    def color_master_entry(self, me, me2, se):
        sha = hashlib.sha1()
        sha.update(me.get_text().encode('utf-8'))
        mesha1 = sha.hexdigest()
        print(repr(mesha1))

        red = 0
        green = 0

        if self.opts.newpwd:
            if me.get_text() != me2.get_text():
                red = 1
            else:
                green = 1

        return # XXX FIXME
        if red:
            me.modify_base(gtk.STATE_NORMAL,
                                     gtk.gdk.color_parse("#FF0000"))
        elif green:
            me.modify_base(gtk.STATE_NORMAL,
                                     gtk.gdk.color_parse("#00FF00"))
        else:
            me.modify_base(gtk.STATE_NORMAL, None);


    def quit(self):
        self.clip.clear()
        gtk.main_quit()

    def len_change(self, adj):
        # print('adj: {}'.format(dir(adj)))
        self.opts.len = int(adj.get_value())

    def errmsg(self, msg):
            errdialog = gtk.MessageDialog(parent=self.wnd, flags=0,
                type=gtk.MESSAGE_ERROR,
                buttons=gtk.BUTTONS_CLOSE, message_format=None)
            errdialog.set_markup(msg)
            errdialog.run()
            errdialog.destroy()

    def match_func(self, completion, key, iter):
        model = completion.get_model()
        if model[iter][COL_TEXT] is not None:
            return model[iter][COL_TEXT].find(self.service_entry.get_text()) != -1

    def on_completion_match(self, completion, model, iter):
        service_name = model[iter][COL_TEXT]
        self.service_entry.set_text(service_name)
        self.service_entry.set_position(-1)
        print("XXX Setting len={}".format(self.cfg.get_len(service_name)))
        self.len_adj.set_value(self.cfg.get_len(service_name))
        print("XXX value set={}".format(self.len_adj.get_value()))

    def set_words(self, words):
        model = self.service_entry.get_completion().get_model()
        model.clear()
        for word in words:
            model.append([word])

    def expanded_cb(self, expander, params):
        self.opts.newpwd = expander.get_expanded()

    def __init__(self, pwfun, options, cfg):
        self.pwfun = pwfun
        self.opts = options
        self.cfg = cfg
        self.clip = gtk.Clipboard()

        # self.wnd = wnd = gtk.Window(type=gtk.WINDOW_TOPLEVEL)
        self.wnd = wnd = gtk.Window()
        wnd.connect("destroy", lambda w: self.quit())
        wnd.set_title("mkpass")
        wnd.set_size_request(320, -1)
        vb = gtk.VBox(False, 0)

        master_label = gtk.Label("Enter Master Password:")
        vb.pack_start(master_label, False, False, 0)
        master_label.show()

        master_entry = gtk.Entry()
        master_entry.set_max_length(32)
        master_entry.set_text("")
        master_entry.set_visibility(0)
        vb.pack_start(master_entry, False, False, 0)
        master_entry.show()

        # expander = gtk.Expander(None)
        expander = gtk.Expander()
        double_check = gtk.Label("Double check master password")
        expander.set_label_widget(double_check)
        double_check.show()

        master2_entry = gtk.Entry()
        master2_entry.set_max_length(32)
        master2_entry.set_text("")
        master2_entry.set_visibility(0)

        expander.add(master2_entry)
        master2_entry.show()
        vb.pack_start(expander, False, False, 0)
        expander.connect('notify::expanded', self.expanded_cb)
        if self.opts.newpwd is not None:
            expander.set_expanded(self.opts.newpwd)
        expander.show()

        sep = gtk.HSeparator()
        vb.pack_start(sep, False, True, 5)
        sep.show()

        service_label = gtk.Label("Enter Service Name:")
        vb.pack_start(service_label, False, False, 0)
        service_label.show()

        service_entry = gtk.Entry()
        service_entry.set_max_length(64)
        service_entry.set_text("")
        service_entry.set_visibility(1)
        self.service_entry = service_entry

        completion = gtk.EntryCompletion()
        completion.set_match_func(self.match_func)
        completion.connect("match-selected",
                self.on_completion_match)
        completion.set_model(gtk.ListStore(str))
        completion.set_text_column(COL_TEXT)
        service_entry.set_completion(completion)

        vb.pack_start(service_entry, False, False, 0)
        service_entry.show()

        master_entry.connect("changed",
            lambda w: self.color_master_entry(master_entry,
                                              master2_entry,
                                              service_entry))
        master2_entry.connect("changed",
            lambda w: self.color_master_entry(master_entry,
                                              master2_entry,
                                              service_entry))
        expander.connect('notify::expanded',
            lambda w,e: self.color_master_entry(master_entry,
                                              master2_entry,
                                              service_entry))

        hb1 = gtk.HBox()
        pwdlen_label = gtk.Label("Password length:")
        adj = gtk.Adjustment(self.opts.len, 4, 24+1, 1, 1, 1)
        # scale = gtk.HScale(adj)
        scale = gtk.HScale()
        adj.connect("value_changed", self.len_change)
        self.len_adj = adj
        scale.set_digits(0)
        hb1.pack_start(pwdlen_label, False, False, 10)
        pwdlen_label.show()
        hb1.pack_start(scale, True, True, 0)
        scale.show()
        hb1.show()
        vb.pack_start(hb1, False, False, 0)

        sep = gtk.HSeparator()
        vb.pack_start(sep, False, True, 5)
        sep.show()

        close_button = gtk.Button(stock=gtk.STOCK_CLOSE)
        close_button.connect("clicked", lambda w: self.quit())
        close_button.show()

        copy_button = gtk.Button(stock=gtk.STOCK_COPY)
        copy_button.connect("clicked",
            lambda w: self.copy(master_entry, master2_entry, service_entry))
        copy_button.show()

        hb2 = gtk.HBox()
        hb2.pack_start(copy_button, True, True, 0)
        hb2.pack_start(close_button, True, True, 0)
        hb2.show()
        vb.pack_end(hb2, False, False, 0)

        wnd.add(vb)
        vb.show()
        wnd.show()

    def run(self):
        self.set_words(self.cfg.get_names())
        gtk.main()
        return 0

################################################################################

usage = "usage: %prog [options] URI"
parser = optparse.OptionParser(usage=usage)
parser.add_option('-n', '--new',
        dest = 'newpwd',
        action = 'store_true',
        help = 'Double ask the master password')

parser.add_option('-g', '--gui',
        dest = 'gui',
        action = 'store_true',
        help = 'Run in GUI mode')

parser.add_option('-t', '--tui',
        dest = 'tui',
        action = 'store_true',
        help = 'Run in text mode')

parser.add_option('-l', '--length',
        dest = 'len',
        type = 'int',
        default = 8,
        action = 'store',
        help = 'Password length')

(options, arguments) = parser.parse_args()
cfg = ConfigDB()

if "DISPLAY" in os.environ and not options.tui:
    options.gui = True

if options.gui:
    import gi
    gi.require_version('Gtk', '3.0')
    from gi.repository import Gtk as gtk
    #import pygtk
    #pygtk.require('2.0')
    #import gtk
    sys.exit(MakePassGUI(mkpass, options, cfg).run())
else:
    import getpass
    sys.exit(MakePassTUI(mkpass, options, cfg).run())
