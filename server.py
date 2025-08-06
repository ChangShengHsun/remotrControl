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

def day_micro():
    return int(time.time() % 86400 * 1_000_000)
    #as a notice, if you test lightdance over midnight, you have to run it again.

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
    with clients_lock:
        if client in clients:
            clients.remove(client)
    client.close()
    print(f"SERVER: client {addr} disconnected")

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
        

def parse_input(user_input):
    #split user_input and divide cmd as well as args
    tokens = user_input.split()
    if not tokens:
        return None
    cmd = tokens[0]
    args = []

    # Restrict play command to: play <seconds>
    if cmd == "play":
        if len(tokens) != 2:
            print("Usage: play <seconds>")
            return None
        try:
            seconds = int(tokens[1])
        except ValueError:
            print("Invalid seconds value")
            return None
        args = ["playerctl", "play", str(seconds * 1000)]  # milliseconds
        return {
            "type": "command",
            "args": args,
            "t_send": day_micro()
        }

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

        # Add t_send for play command
        return {
            "type": "command",
            "args": args,
            "t_send": day_micro()
        }

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

        #turn it into json style
        if command:
            json_str = json.dumps(command) + '\n'
            with clients_lock:
                for c in clients:
                    try:
                        c.sendall(json_str.encode())                        

                        print("SERVER: send json")
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