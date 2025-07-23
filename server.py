import socket

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind(('192.168.1.83', 4000))

server.listen(5)

while True:
    client, addr = server.accept()
    print(client.recv(1024).decode())
    server.send('hello world'.encode())