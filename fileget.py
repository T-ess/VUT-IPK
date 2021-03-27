import socket
import argparse
from urllib.parse import urlparse
import re

##### argument parsing #####
# fileget -n NAMESERVER -f SURL

parser = argparse.ArgumentParser(description='Parse the fileget command.')
parser.add_argument('-n', required=True)
parser.add_argument('-f', required=True)
args = parser.parse_args()

surl = urlparse(args.f)
if surl.scheme != 'fsp':
    exit("Use the fsp protocol.")
nameserver = args.n.split(':', 1)
nameserver[1] = int(nameserver[1])

##### Name Service Protocol #####

s_nsp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s_nsp.sendto(bytes(f"WHEREIS {surl.hostname}\r\n", 'utf-8'), (nameserver[0], nameserver[1])) #TODO: try expect

while True:
    fileserver = s_nsp.recv(30).decode('utf-8')
    if len(fileserver) > 0:
        break

#TODO: timeout
s_nsp.close()

if fileserver[0:3] == 'ERR':
    exit("NSP request error")

fileserver = fileserver[3:].split(":", 1) # ip address + port
fileserver[1] = int(fileserver[1]) # port

##### File Service Protocol #####
s_fsp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s_fsp.connect((fileserver[0], fileserver[1])) #TODO: try expect
url_file = '.' + surl.path

get_request = f"GET {url_file} FSP/1.0\r\nHostname: {surl.hostname}\r\nAgent: xburia28\r\n\r\n"
s_fsp.sendall(bytes(get_request, 'utf-8'))

getanswer = ''
filelength = 0
parseHeader = True
get_err = False
msg_err = ''

while True:

    getanswer = s_fsp.recv(100)

    if parseHeader: # first run of the loop

        # check server answer for success/error
        header = getanswer.splitlines()
        if header[0] != b'FSP/1.0 Success':
            get_err = True

        # save file length from header
        filelength = int(re.search(rb'\d+', header[1]).group())

        #remove the header
        getanswer = re.split(rb'(?:\r\n|\r(?!\n)|(?<!\r)\n){2}', getanswer)
        getanswer = bytes(getanswer[1])

        # create the file
        if not get_err:
            f = open(url_file, "xb") #TODO: error handling

    # the whole answer was received
    if (not getanswer) and (not parseHeader):
        if get_err:
            s_fsp.close()
            exit(msg_err)
        else:
            f.close()
        break

    # add received text to file/error message
    if get_err:
        getanswer = getanswer.decode("utf-8")
        msg_err += getanswer
    else:
        f.write(getanswer)
    
    parseHeader = False

s_fsp.close()
