#!/usr/bin/env python
#import socket module
from socket import *
import time 
while True:
	serverSocket = socket(AF_INET, SOCK_STREAM) 
	#Allows to avoid socket.error: [Errno 48] Address already in use
	serverSocket.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1) 
	#Prepare a sever socket 
	serverSocket.bind(('',5125))
	serverSocket.listen(1)
	
	count = 0
	#Establish the connection 
	print 'Ready to serve...' 
	connectionSocket, addr = serverSocket.accept()

	while True:
		#Receive message from a client and determine the file name.
       		connectionSocket.send('read')        
		message = connectionSocket.recv(1440) 
		count = count + 1
		if not message: break
		print count #message
	#Close serverSocket
	serverSocket.close()
serverSocket.close()
