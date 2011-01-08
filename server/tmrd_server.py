#!/usr/bin/python

#
#   tmrd - a simple sound delayer
#   Copyright (C) 2009-2011 Ian Martins
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import subprocess, os, signal, sys
import BaseHTTPServer, getopt

VERSION = "1.0"

class TmrdRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_POST(s):
        tmrd_pid = s.get_pid()
        if tmrd_pid == None:
            s.send_hdr("text")
            s.wfile.write("fail")
        else:
            if s.path == '/increase':
                os.kill(tmrd_pid, signal.SIGUSR1)
            elif s.path == '/decrease':
                os.kill(tmrd_pid, signal.SIGUSR2)
            s.send_hdr("text")
            s.wfile.write("ok");

    def do_GET(s):
        if s.path == '/screen.css':
            s.send_hdr("css")
            for line in open('screen.css', 'r'):
                s.wfile.write(line)
        elif s.path == '/mobile.css':
            s.send_hdr("css")
            for line in open('mobile.css', 'r'):
                s.wfile.write(line)
        else:
            s.send_hdr("html")
            for line in open('index.html', 'r'):
                s.wfile.write(line)

    def send_hdr(s, type):
            s.send_response(200)
            s.send_header("Content-type", "text/"+type)
            s.end_headers()
                    
    # get tmrd pid
    def get_pid(s):
        ps_cmd = subprocess.Popen('ps -e | grep tmrd', shell=True, stdout=subprocess.PIPE)
        ps_out = ps_cmd.stdout.read()
        if ps_out == '':
            return None
        return int(ps_out.split(' ')[0])
        

def printHelp():
    print "Usage: tmrd_server [OPTION]"
    print "Options:"
    print "-p, --port <arg>    set port"
    print "-h, --help          display this help text and exit"
    print "-v, --version       display version and exit"

# parse command line args
print "tmrd_server.py v" + VERSION
try:
    opts, args = getopt.getopt(sys.argv[1:], "p:hv", ["port=", "help", "version"])
except getopt.GetoptError:
    printHelp()
    sys.exit(2)
for opt, arg in opts:
    if opt in ("-p", "--port"):
        port = arg
    elif opt in ("-h", "--help"):
        printHelp()
        sys.exit()
    elif opt in ("-v", "--version"):
        sys.exit()

# start the server
print 'Binding to port', int(port)
server_address = ('', int(port))
httpd = BaseHTTPServer.HTTPServer(server_address, TmrdRequestHandler)
httpd.serve_forever()
