#!/usr/bin/python

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

import sys
import sha
import base64
import optparse

################################################################################

# Generates base64 encoded SHA1 out of master and slave passwords
def mkpass(master, slave, len=8):
    shao = sha.new(master + slave)
    hash = shao.digest()
    return base64.b64encode(hash)[:len]

################################################################################

class MakePassTUI:
    def __init__(self, pwfun, options):
        self.pwfun = pwfun
        self.opts = options

    def run(self):
        # Prompt for the master password and service specific info
        try:
            master = getpass.getpass("Master password: ")
            if self.opts.newpwd \
                and master != getpass.getpass("ReEnter Master password: "):
                print "Error: Passwords mismatch!"
                return 1
            service = raw_input("Service: ")
            # Print out the password we've just generated
            print "Password: %s" % self.pwfun(master, service, self.opts.len)
        except KeyboardInterrupt, e:
            return 1
        except EOFError, e:
            return 1

################################################################################

class MakePassGUI:
    def copy(self, me, me2, se):
        if self.opts.newpwd and me.get_text() != me2.get_text():
            self.errmsg("Passwords mismatch!")
        else:
            master = me.get_text()
            service = se.get_text()
            self.clip.set_text(self.pwfun(master, service, self.opts.len))

    def quit(self):
        self.clip.clear()
        gtk.main_quit()

    def len_change(self, adj):
        self.opts.len = int(adj.value)

    def errmsg(self, msg):
            errdialog = gtk.MessageDialog(parent=self.wnd, flags=0,
                type=gtk.MESSAGE_ERROR,
                buttons=gtk.BUTTONS_CLOSE, message_format=None)
            errdialog.set_markup(msg)
            errdialog.run()
            errdialog.destroy()

    def __init__(self, pwfun, options):
        self.pwfun = pwfun
        self.opts = options
        self.clip = gtk.Clipboard()

        self.wnd = wnd = gtk.Window(type=gtk.WINDOW_TOPLEVEL)
        wnd.connect("destroy", lambda w: self.quit())
        wnd.set_title("mkpass")
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

        master2_label = gtk.Label("Re-Enter Master Password:")
        if self.opts.newpwd:
            vb.pack_start(master2_label, False, False, 0)
            master2_label.show()

        master2_entry = gtk.Entry()
        master2_entry.set_max_length(32)
        master2_entry.set_text("")
        master2_entry.set_visibility(0)
        if self.opts.newpwd:
            vb.pack_start(master2_entry, False, False, 0)
            master2_entry.show()

        service_label = gtk.Label("Enter Service Name:")
        vb.pack_start(service_label, False, False, 0)
        service_label.show()

        service_entry = gtk.Entry()
        service_entry.set_max_length(32)
        service_entry.set_text("")
        service_entry.set_visibility(1)
        vb.pack_start(service_entry, False, False, 0)
        service_entry.show()

        sep = gtk.HSeparator()
        vb.pack_start(sep, False, False, 0)
        sep.show()

        hb1 = gtk.HBox()
        pwdlen_label = gtk.Label("Password length:")
        adj = gtk.Adjustment(self.opts.len, 4, 24+1, 1, 1, 1)
        scale = gtk.HScale(adj)
        adj.connect("value_changed", self.len_change)
        scale.set_digits(0)
        hb1.pack_start(pwdlen_label, False, False, 10)
        pwdlen_label.show()
        hb1.pack_start(scale, True, True, 0)
        scale.show()
        hb1.show()
        vb.pack_start(hb1, False, False, 0)

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
        vb.pack_start(hb2, False, False, 0)

        wnd.add(vb)
        vb.show()
        wnd.show()

    def run(self):
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

parser.add_option('-l', '--length',
        dest = 'len',
        type = 'int',
        default = 8,
        action = 'store',
        help = 'Password length')

(options, arguments) = parser.parse_args()

if options.gui:
    import pygtk
    pygtk.require('2.0')
    import gtk
    sys.exit(MakePassGUI(mkpass, options).run())
else:
    import getpass
    sys.exit(MakePassTUI(mkpass, options).run())

