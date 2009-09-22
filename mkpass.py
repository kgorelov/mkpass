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
import getpass
import optparse

################################################################################

# Generates base64 encoded SHA1 out of master and slave passwords
def mkpass(master, slave):
    shao = sha.new(master + slave)
    hash = shao.digest()
    return base64.b64encode(hash)[:8]

################################################################################

class MakePassTUI:
    def __init__(self, pwfun, recheck=0):
        self.pwfun = pwfun
        self.recheck = recheck

    def run(self):
        # Prompt for the master password and service specific info
        try:
            master = getpass.getpass("Master password: ")
            if self.recheck \
                and master != getpass.getpass("ReEnter Master password: "):
                print "Error: Passwords mismatch!"
                return 1
            service = raw_input("Service: ")
            # Print out the password we've just generated
            print "Password: %s" % self.pwfun(master, service)
        except KeyboardInterrupt, e:
            return 1
        except EOFError, e:
            return 1

################################################################################

class MakePassGUI:
    def copy(self, me, me2, se):
        #print "Master: %s / Slave: %s" % (me.get_text(), se.get_text())
        if self.recheck and me.get_text() != me2.get_text():
            errdialog = gtk.MessageDialog(parent=None, flags=0,
                type=gtk.MESSAGE_ERROR,
                buttons=gtk.BUTTONS_CLOSE, message_format=None)
            #errlabel = gtk.Label("Passwords mismatch!")
            #errdialog.pack_start(errlabel, True, True, 0)
            errdialog.set_markup("Passwords mismatch!")
            errdialog.run()
            errdialog.destroy()
        else:
            self.clip.set_text(self.pwfun(me.get_text(), se.get_text()))

    def quit(self):
        print "Quitting..."
        self.clip.clear()
        gtk.main_quit()

    def __init__(self, pwfun, recheck=0):
        self.pwfun = pwfun
        self.recheck = recheck
        self.clip = gtk.Clipboard()

        master_label = gtk.Label("Enter Master Password:")
        master_label.show()

        master_entry = gtk.Entry()
        master_entry.set_max_length(50)
        master_entry.set_text("")
        master_entry.set_visibility(0)
        master_entry.show()

        master2_label = gtk.Label("Re-Enter Master Password:")
        master2_label.show()

        master2_entry = gtk.Entry()
        master2_entry.set_max_length(50)
        master2_entry.set_text("")
        master2_entry.set_visibility(0)
        if recheck:
            master2_entry.show()

        service_label = gtk.Label("Enter Service Name:")
        service_label.show()

        service_entry = gtk.Entry()
        service_entry.set_max_length(50)
        service_entry.set_text("")
        service_entry.set_visibility(1)
        service_entry.show()

        dialog = gtk.Dialog(title="mkpass", parent=None,
                            flags=gtk.DIALOG_MODAL | gtk.WINDOW_TOPLEVEL,
                            buttons=None)
        dialog.set_flags(gtk.FLOATING)
        #dialog.set_flags(gtk.gdk.WINDOW_STATE_ABOVE)
        #dialog.set_size_request(200, 100)
        dialog.set_geometry_hints(dialog, max_width=200, max_height=200,
                base_width=200, base_height=200)
        dialog.set_position(gtk.WIN_POS_CENTER)

        close_button = gtk.Button(stock=gtk.STOCK_CLOSE)
        close_button.connect("clicked", lambda w: self.quit())
        close_button.show()

        copy_button = gtk.Button(stock=gtk.STOCK_COPY)
        copy_button.connect("clicked",
            lambda w: self.copy(master_entry, master2_entry, service_entry))
        copy_button.show()

        dialog.action_area.pack_start(copy_button, True, True, 0)
        dialog.action_area.pack_start(close_button, True, True, 0)
        dialog.vbox.pack_start(master_label, True, True, 0)
        dialog.vbox.pack_start(master_entry, True, True, 0)
        if recheck:
            dialog.vbox.pack_start(master2_label, True, True, 0)
            dialog.vbox.pack_start(master2_entry, True, True, 0)
        dialog.vbox.pack_start(service_label, True, True, 0)
        dialog.vbox.pack_start(service_entry, True, True, 0)
        dialog.vbox.pack_start(gtk.VRuler(), True, True, 0)
        dialog.connect("close", lambda w: self.quit())

        dialog.show()

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


(options, arguments) = parser.parse_args()

if options.gui:
    import pygtk
    pygtk.require('2.0')
    import gtk
    sys.exit(MakePassGUI(mkpass, options.newpwd).run())
else:
    sys.exit(MakePassTUI(mkpass, options.newpwd).run())

