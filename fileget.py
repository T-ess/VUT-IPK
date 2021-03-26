import socket
import argparse
from urllib.parse import urlparse


##### argument parsing #####
# fileget -n NAMESERVER -f SURL
# ./fileget -n 147.229.8.12:3333 -f fsp://foo.bar/file.txt

parser = argparse.ArgumentParser(description='Parse the fileget command.')
parser.add_argument('-n', required=True)
parser.add_argument('-f', required=True)
args = parser.parse_args()

surl = urlparse(args.f)
nameserver = args.n.split(':', 1)
nameserver[1] = int(nameserver[1])

if nameserver[1] < 1024 or nameserver[1] > 65535:
    print("uh oh")

### NSP ###

s_nsp = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

s_nsp.connect((nameserver[0], nameserver[1]))

full_msg = ''

s_nsp.send(bytes("WHEREIS server1 \n", 'utf-8'))
#s.send(bytes("GET soubor1.txt FSP/1.0 \n", 'utf-8'))
#s.send(bytes("Hostname: server1 \n", 'utf-8'))
#s.send(bytes("Agent: xburia28 \n", 'utf-8'))


while True:
    where = s_nsp.recv(30)
    if len(where) > 0:
        print(where)
        str(where)
        break

s_nsp.close()
fileserver = where.decode('utf-8')

if fileserver[0:3] == 'ERR':
    print("uh oh")

fileserver = fileserver[3:].split(":", 1)

print("yay")

### FSP ###
s_fsp = socket.socket(socket.AF_INET, socket.SOCK_STREAM)