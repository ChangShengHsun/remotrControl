import socket
import json
#import threading
HOST = '192.168.1.83'
PORT = 5000

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)#socket.socket()
server.bind((HOST, PORT))#.bind(IPaddress, port)

server.listen(20) #start to listen, .listen(max amount clients in line)

print("ready to connect")
client, addr = server.accept() # accept a client
data = client.recv(1024).decode()
print(data) # print the message receive from the client, .recv(maximum bytes of receive data)
#client.send('hello world'.encode()) # send hello world

def parse_input(user_input):
    #split user_input and divide cmd as well as args
    tokens = user_input.split()
    if not tokens:
        return None
    cmd = tokens[0]
    args = []

    #deal with different command
    if cmd == "play":
        args = ["playerctl", cmd]
        if "-d" in tokens:
            idx = tokens.index("-d")
            if idx + 1 < len(tokens):
                args.append(tokens[idx+1])
        elif "-s" in tokens:
            idx = tokens.index("-s")
            if idx + 1 < len(tokens):
                try:
                    args.append(str(int(tokens[idx+1])*1000))
                except:
                    print("Invalid time format")
                    return None

    elif cmd in ["pause", "stop", "restart", "quit"]:
        args = ["playerctl", cmd]
    elif cmd == "list":
        args = ["list"]
    elif cmd == "ledtest":
        args = ["ledtest"] + tokens[1:]
    elif cmd == "oftest":
        args = ["oftest"] + tokens[1:]
    else:
        print("Not support")
        return None

    return {
        "type": "command",
        "args": args
    }


while True:
    try:
        user_input = input("SERVER: Enter:")# get userinput
        #if input is exit or quit, terminate the program
        if user_input.strip().lower() in ["exit", "quit"]:
            print("SERVER: quit")
            break
        command = parse_input(user_input)

        #turn it into json style
        if command:
            json_str = json.dumps(command) + '\n'
            client.sendall(json_str.encode())
            print("SERVER: send json")

    except KeyboardInterrupt:
        break
    except Exception as e:
        print("SERVER: error {e}")
        break

client.close()
server.close()
print("SERVER: finish connection")