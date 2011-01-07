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

import subprocess, os
import BaseHTTPServer

class TmrdRequestHandler(BaseHTTPServer.BaseHTTPRequestHandler):
    def do_POST(s):
        tmrd_pid = s.get_pid()
        if tmrd_pid == None:
            s.send_hdr("text")
            s.wfile.write("fail")
        else:
            if s.path == '/increase':
                os.kill(tmrd_pid, 34)
            elif s.path == '/decrease':
                os.kill(tmrd_pid, 35)
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
        

# start the server
server_address = ('', 8000)
httpd = BaseHTTPServer.HTTPServer(server_address, TmrdRequestHandler)
httpd.serve_forever()
