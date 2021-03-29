#!/usr/bin/env python3.8
"""VUT FIT - IPK, Project 1, client downloading files from server. 
Author: Tereza Burianova (xburia28@vutbr.cz)
"""

import socket
import argparse
from urllib.parse import urlparse
import re
import sys

def argument_parse():
    """Parses the parameters: fileget -n NAMESERVER -f SURL
    """
    global surl
    global nameserver
    #* read arguments
    parser = argparse.ArgumentParser(description='Parse the fileget command.')
    parser.add_argument('-n', required=True)
    parser.add_argument('-f', required=True)
    args = parser.parse_args()
    #* parse SURL 
    surl = urlparse(args.f)
    if surl.scheme != 'fsp':
        exit("Use the fsp protocol.")
    #* parse NAMESERVER - ip and port
    nameserver = args.n.split(':', 1)
    nameserver[1] = int(nameserver[1]) # port to integer

def fsp(server, file, path):
    s_fsp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    #* connect and send GET request
    s_fsp.connect((fileserver[0], fileserver[1])) #TODO: try expect
    get_request = f"GET {surl_path} FSP/1.0\r\nHostname: {surl.hostname}\r\nAgent: xburia28\r\n\r\n"
    s_fsp.sendall(bytes(get_request, 'utf-8'))

    getanswer = ''
    filelength = 0
    parseHeader = True
    get_err = False
    msg_err = ''

    #* receive and save results
    while True:

        getanswer = s_fsp.recv(512)

        if parseHeader: #* first run of the loop

            #* check server answer for success/error
            #TODO: exception pokud prijde prazdna odpoved - no answer from server a exit
            if getanswer == b'':
                exit("uh oh")
            header = getanswer.splitlines()
            if header[0] != b'FSP/1.0 Success':
                get_err = True

            #* save file length from header
            filelength = int(re.search(rb'\d+', header[1]).group())

            #* remove the header
            getanswer = re.split(rb'(?:\r\n|\r(?!\n)|(?<!\r)\n){2}', getanswer)
            getanswer = bytes(getanswer[1])

            #* create the file
            if not get_err:
                f = open(surl_file, "wb") #TODO: error handling

        #* if the whole answer was received
        if (not getanswer) and (not parseHeader):
            if get_err:
                s_fsp.close()
                exit(msg_err)   
            else:
                f.close()
            break

        #* add received text to file/error message
        if get_err:
            getanswer = getanswer.decode("utf-8")
            msg_err += getanswer
        else:
            f.write(getanswer)
        
        parseHeader = False

    s_fsp.close()

argument_parse()

####* Name Service Protocol #####

s_nsp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s_nsp.sendto(bytes(f"WHEREIS {surl.hostname}\r\n", 'utf-8'), (nameserver[0], nameserver[1])) #TODO: try expect
fileserver = s_nsp.recv(32).decode('utf-8')
#TODO: timeout
s_nsp.close()

if fileserver == '':
    exit("NSP server error")
if fileserver[0:3] == 'ERR':
    exit("NSP request error")

fileserver = fileserver[3:].split(":", 1) # ip address + port
fileserver[1] = int(fileserver[1]) # port

#* get file path and name
surl_path = '.' + surl.path
surl_file = surl_path.split("/")
surl_file = surl_file[-1]
if surl_path == "./" + surl_file:
    surl_path = surl_file


fsp(surl.hostname, surl_file, surl_path)
