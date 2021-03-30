#!/usr/bin/env python3.8
"""VUT FIT - IPK, Project 1, client downloading files from server. 
Author: Tereza Burianova (xburia28@vutbr.cz)
"""

import socket
import argparse
from urllib.parse import urlparse
import re
import sys
import os

def argument_parse():
    """Parses the parameters: fileget -n NAMESERVER -f SURL
    """
    global surl
    global nameserver
    #* read arguments
    parser = argparse.ArgumentParser(description='Parse the fileget command.')
    parser.add_argument('-n', required=True)
    parser.add_argument('-f', required=True)
    try:
        args = parser.parse_args()
    except:
        sys.exit("Error parsing the arguments. Usage: fileget -n NAMESERVER -f SURL")
    #* parse SURL 
    try:
        surl = urlparse(args.f)
    except:
        sys.exit("Invalid SURL.")
    if surl.scheme != 'fsp':
        sys.exit("Use the fsp protocol.")
    #* parse NAMESERVER - ip and port
    nameserver = args.n.split(':', 1)
    try:
        nameserver[1] = int(nameserver[1]) # port to integer
    except:
        sys.exit("Port must be a numeric value.")

def file_from_path(path):
    """Gets the filename from a path.
    Args:
        path (string): path to file
    Returns:
        [string]: filename
    """
    filename = path.split("/")
    filename = filename[-1]
    return filename

def copy_filename(duplicate_file):
    """Changes the name of the downloaded server Windows-style if a file with the same name already exists.
    Args:
        duplicate_file (string): name of the file on the server
    Returns:
        string: new unique filename
    """
    while os.path.exists(duplicate_file):
        duplicate_file = duplicate_file.rsplit('.', 1) # split file name and extension if present
        if re.search(r"\([0-9]*\)", duplicate_file[0]): # if a copy of this file already exists
            filenum = re.search(r"\d*(?=\))", duplicate_file[0]).group(0) # match digits inside ( )
            filenum = int(filenum)
            filenum += 1
            filenum = '(' + str(filenum) + ')'
            duplicate_file[0] = re.sub(r"\([0-9]*\)", filenum, duplicate_file[0]) # increment digit inside ( ) 
            duplicate_file[0] = duplicate_file[0] + '.'
        else:
            duplicate_file[0] = duplicate_file[0] + '(1)' + '.'
        duplicate_file = ''.join(duplicate_file)
    return duplicate_file


def fsp(server, file, path, all=False):
    """FSP - downloads the specified file from the server.
    Args:
        server (string): fileserver
        file (string): name of downloaded file
        path (string): path of downloaded file
        all (bool, optional): True if all files from the server are being downloaded. Defaults to False.
    """
    #* rename file if already exists
    if file != 'index':
        file = copy_filename(file)
    #* connect and send GET request
    s_fsp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s_fsp.settimeout(600)
    try:
        s_fsp.connect((fileserver[0], fileserver[1]))
    except:
        sys.exit("Error connecting to the server.")
    get_request = f"GET {path} FSP/1.0\r\nHostname: {server}\r\nAgent: xburia28\r\n\r\n"
    try:
        s_fsp.sendall(bytes(get_request, 'utf-8'))
    except:
        sys.exit("Error sending the GET request.")

    getanswer = ''
    filelength = 0
    parseHeader = True
    get_err = False
    msg_err = ''

    #* receive and save results
    while True:
        try:
            getanswer = s_fsp.recv(512)
        except:
            sys.exit("Error receiving the data.")
        if parseHeader: #* first run of the loop
            #* check server answer for success/error
            if getanswer == b'':
                s_fsp.close()
                sys.exit("Unexpected response from the server.")
            header = getanswer.splitlines()
            if header[0] != b'FSP/1.0 Success':
                get_err = True
            #* save file length from header
            filelength = int(re.search(rb'\d+', header[1]).group())
            #* remove the header
            getanswer = re.split(rb'(?:\r\n){2}', getanswer)
            getanswer = bytes(getanswer[1])
            #* create the file
            if not get_err:
                try:
                    f = open(file, "wb")
                except:
                    s_fsp.close()
                    sys.exit("Cannot create the local file.")

        #* if the whole answer was received
        if (not getanswer) and (not parseHeader):
            if get_err:
                s_fsp.close()
                if all:
                    print(msg_err)
                    return
                sys.exit(msg_err)
            else:
                f.close()
                if os.stat(file).st_size != filelength:
                    if os.path.exists(file):
                        os.remove(file)
                    sys.exit("The connection was unexpectedly terminanted. The file was removed.")
            break
        #* add received text to file/error message
        if get_err:
            getanswer = getanswer.decode("utf-8")
            msg_err += getanswer
        else:
            try:
                f.write(getanswer)
            except:
                s_fsp.close()
                if os.path.exists(file):
                    os.remove(file)
                sys.exit("Unable to write into the local file. The file was removed.")
        #* set first run to finished
        parseHeader = False

    s_fsp.close()




argument_parse()

####* Name Service Protocol #####

s_nsp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
try:
    s_nsp.sendto(bytes(f"WHEREIS {surl.hostname}\r\n", 'utf-8'), (nameserver[0], nameserver[1]))
    s_nsp.settimeout(5)
    fileserver = s_nsp.recv(32).decode('utf-8')
except:
    sys.exit("Error in UDP connection. Check the IP address.")
s_nsp.close()

if (fileserver == '') or (fileserver[0:3] == 'ERR'):
    sys.exit("Unable to get the file server IP address.")

fileserver = fileserver[3:].split(":", 1) # ip address + port
try:
    fileserver[1] = int(fileserver[1]) # port
except:
    sys.exit("Port must be a numeric value.")

#* get file path and name
getall = False
surl_path = '.' + surl.path
surl_file = file_from_path(surl_path)
if surl_path == "./" + surl_file:
    surl_path = surl_file

if surl_file == '*':
    if surl_file != surl_path:
        sys.exit("Invalid SURL. GETALL ('*') can only be called from the root folder.")
    getall = True

if getall:
    fsp(surl.hostname, "index", "index", True)
    try:
        f_index = open('index', 'r')
    except:
        sys.exit('Unable to open the "index" file.')

    for line in f_index:
        line_file = file_from_path(line.strip())
        fsp(surl.hostname, line_file, line.strip())
else:
    fsp(surl.hostname, surl_file, surl_path)
