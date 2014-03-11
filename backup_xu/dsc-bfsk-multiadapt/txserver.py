#!/usr/bin/env python
#import socket module
from socket import * 
import sys

#while True:
serverSocket = socket(AF_INET, SOCK_STREAM) 
#Allows to avoid socket.error: [Errno 48] Address already in use
serverSocket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1) 
#Prepare a sever socket 
serverSocket.bind(('',5123))
serverSocket.listen(1)
x = 0
f = open('onetwentyfourpackets.txt') 
outputdata = f.read()
#Establish the connection 
print 'Ready to serve...' 
connectionSocket, addr = serverSocket.accept()
while True:
        #Receive message from a client and determine the file name.
        message = connectionSocket.recv(1440)
        if not message: break
        # print message
        if x >= len(outputdata):
            print "All Packets Sent"
            break
      	#Open the file it asked and read the data inside.
    	connectionSocket.send(outputdata[x:x+1440])
        sys.stderr.write('.')
        x += 1440

#Close serverSocket
serverSocket.close()
#serverSocket.close()
