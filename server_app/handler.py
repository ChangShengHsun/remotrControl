import json
from utils import day_micro

# handle the time synchronization messages from clients
# This function will be called in a separate thread for each client connection.
def handle_client(client, addr, clients, clients_lock):
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
                        "t_3": day_micro(),
                        "t_1": t_1
                    }
                    try:
                        client.sendall((json.dumps(sync_resp) + '\n').encode())
                        print(f"SERVER: sync from {addr}, t_1={t_1}, t_2={t_2}, t_3={sync_resp['t_3']}")
                    except Exception as e:
                        print(f"SERVER: failed sending sync_resp to {addr}: {e}")
        except Exception as e:
            print(f"SERVER: client {addr} error: {e}")
            break
    # Cleanup
    try:
        client.close()
    except Exception:
        pass
    with clients_lock:
        if client in clients:
            clients.remove(client)
    print(f"SERVER: client {addr} disconnected and removed")