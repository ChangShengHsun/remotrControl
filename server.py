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

def time_sync(client, addr, retries = 3):
    for attempt in range(retries):
        success_count = 0
        for i in range(5):# do time sync 5 times and get the average
            try:
                t_send = day_micro()
                sync_msg = {"type": "sync",
                            "t_send": t_send}
                client.sendall((json.dumps(sync_msg) + '\n').encode()) 

                client.settimeout(0.5)
                data = client.recv(1024).decode()
                response = json.loads(data)
                if response.get("type") == "sync_ack":
                    print(f"SERVER: got sync_ack #{i+1} from {addr}")
                    success_count += 1
                else:
                    print((f"SERVER: unexpected response: {response}"))
            except Exception as e:
                print(f"SERVER: sync error with {addr}: {e}")
                break
        if success_count >= 5:
            client.settimeout(None)
            return True
        else:
            print(f"SERVER: retrying time sync with {addr} ({attempt+1}/{retries})")
    print(f"SERVER: time sync with {addr} failed after {retries} attempts")
    client.settimeout(None)
    return False

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

        print(f"SERVER: start time sync with {addr}")
        threading.Thread(target = time_sync, args = (client, addr), daemon = True).start()
        

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