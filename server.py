import socket
import json
import threading
import time
HOST = '0.0.0.0'
PORT = 5000
clients = []
clients_lock = threading.Lock()

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)#socket.socket()
server.bind((HOST, PORT))#.bind(IPaddress, port)
server.listen(20) #start to listen, .listen(max amount clients in line)

# returns the current time in microseconds since the start of the day
def day_micro():
    return int(time.time() % 86400 * 1_000_000)
    #as a notice, if you test lightdance over midnight, you have to run it again.

# handle the time synchronization messages from clients
# This function will be called in a separate thread for each client connection.
def handle_client(client, addr):
    buffer = ""
    while True:
        try:
            data = client.recv(1024).decode()
            if not data:
                break
            buffer += data
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)
                try:
                    msg = json.loads(line)
                except Exception as e:
                    print(f"SERVER: JSON decode error from {addr}: {e}")
                    continue
                if msg.get("type") == "sync":
                    t_1 = msg.get("t_1")
                    t_2 = day_micro()
                    sync_resp = {
                        "type": "sync_resp",
                        "t_2": t_2,
                        "t_3": day_micro()
                    }
                    client.sendall((json.dumps(sync_resp) + '\n').encode())
                    print(f"SERVER: sync from {addr}, t_1={t_1}, t_2={t_2}, t_3={sync_resp['t_3']}")
                # ...handle other message types if needed...
        except Exception as e:
            print(f"SERVER: client {addr} error: {e}")
            break

# Try to accept a client and start handle_client in a new thread.
# This function runs in a loop to continuously accept new clients.
def receiveClient():
    while True:
        try:
            print("ready to connect")
            client, addr = server.accept() # accept a client
            with clients_lock:
                clients.append(client)
        except Exception as e:
            print(f"SERVER: Error {e}")
            break

        print(f"SERVER: start client handler for {addr}")
        threading.Thread(target=handle_client, args=(client, addr), daemon=True).start()
        

#
def parse_input(user_input):
    tokens = user_input.split()
    if not tokens:
        return None
    cmd = tokens[0]
    
    if cmd == "play":
        # Defaults:
        starttime = "0"
        endtime = "0"
        d = "0"
        dd = "0"
        # Allow optional flags: -ss, -end, -d, -dd
        i = 1
        while i < len(tokens):
            token = tokens[i]
            if token == "-ss":
                if i + 1 < len(tokens):
                    starttime = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -ss flag in play command")
                    return None
            elif token == "-end":
                if i + 1 < len(tokens):
                    endtime = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -end flag in play command")
                    return None
            elif token == "-d":
                if i + 1 < len(tokens):
                    d = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -d flag in play command")
                    return None
            elif token == "-dd":
                if i + 1 < len(tokens):
                    dd = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -dd flag in play command")
                    return None
            else:
                print(f"Invalid flag '{token}' in play command")
                return None
        return f"play -ss{starttime} -end{endtime} -d{d} -dd{dd}"
    elif cmd in ["pause", "stop"]:
        if len(tokens) != 1:
            print(f"Usage: {cmd}")
            return None
        return cmd
    elif cmd == "parttest":
        # Defaults:
        rgb = "255"
        channel = "0"
        # Allow optional flags: -rgb, -c
        i = 1
        while i < len(tokens):
            token = tokens[i]
            if token == "-rgb":
                if i + 1 < len(tokens):
                    rgb = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -rgb flag in parttest command")
                    return None
            elif token == "-c":
                if i + 1 < len(tokens):
                    channel = tokens[i + 1]
                    i += 2
                else:
                    print("Missing parameter for -c flag in parttest command")
                    return None
            else:
                print(f"Invalid flag '{token}' in parttest command")
                return None
        return f"parttest -rgb{rgb} -c{channel}"
    else:
        print("Not support")
        return None

client_thread = threading.Thread(target = receiveClient, daemon = True)
client_thread.start()

while True:
    try:
        user_input = input("SERVER: Enter:")# get userinput
        #if input is exit or quit, terminate the program
        if user_input.strip().lower() in ["exit", "quit"]:
            print("SERVER: quit")
            break
        command = parse_input(user_input)
        if command:
            # Send command as a string instead of JSON.
            command_line = command + '\n'
            with clients_lock:
                for c in clients:
                    try:
                        c.sendall(command_line.encode())
                        print("SERVER: send command string")
                    except Exception as e:
                        print(f"SERVER: failed to send due to {e}")
    except KeyboardInterrupt:
        break
    except Exception as e:
        print(f"SERVER: error {e}")
        break

with clients_lock:
    for c in clients:
        c.close()
server.close()
print("SERVER: finish connection")