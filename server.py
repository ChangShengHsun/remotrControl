import socket

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)#socket.socket()
server.bind(('192.168.1.83', 4000))#.bind(IPaddress, port)

server.listen(20) #start to listen, .listen(max amount clients in line)

while True:
    client, addr = server.accept() # accept a client
    print(client.recv(1024).decode()) # print the message receive from the client, .recv(maximum bytes of receive data)
    client.send('hello world'.encode()) # send hello world