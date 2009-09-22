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

# Parse command line options
usage = "usage: %prog [options] URI"
parser = optparse.OptionParser(usage=usage)
parser.add_option('-n', '--new',
        dest = 'newpwd',
        action = 'store_true',
        help = 'Double ask the master password')

(options, arguments) = parser.parse_args()

# Prompt for the master password and service specific info
try:
    master = getpass.getpass("Master password: ")
    if options.newpwd and master != getpass.getpass("ReEnter Master password: "):
        print "Error: Passwords mismatch!"
        sys.exit(1)
    service = raw_input("Service: ")
except KeyboardInterrupt, e:
    sys.exit(1)
except EOFError, e:
    sys.exit(1)


# Generate SHA1 out of this and base64 encode it to get something printable
shao = sha.new(master + service)
hash = shao.digest()
pwd = base64.b64encode(hash)[:8]

# Print out the password we've just generated
print "Password: %s" % pwd

