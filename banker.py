import socket
import threading

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
    except:
        ip = "127.0.0.1"
    finally:
        s.close()
    return ip

HOST = get_ip()
PORT = 65432
clients = {} 
total_pot = 0

def broadcast(message):
    # C++ needs a null terminator (\0) to recognize the end of a string
    formatted_msg = f"{message}\0".encode()
    for client_socket in list(clients.keys()):
        try:
            client_socket.send(formatted_msg)
        except:
            del clients[client_socket]

def handle_player(conn, addr):
    global total_pot
    try:
        # Get player name
        name = conn.recv(1024).decode().strip('\x00')
        clients[conn] = name
        print(f"PLAYER JOINED: {name}")
        broadcast(f"Pot: ${total_pot} ({name} joined)")

        while True:
            data = conn.recv(1024).decode().strip('\x00')
            if not data: break
            
            if data.isdigit():
                bet = int(data)
                total_pot += bet
                print(f"{name} bet ${bet}. Total: ${total_pot}")
                broadcast(f"Pot: ${total_pot} | Last: {name} (${bet})")
    except:
        pass
    finally:
        conn.close()

def start():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen()
    print(f"BANKER ACTIVE ON {HOST}:{PORT}")
    while True:
        conn, addr = server.accept()
        threading.Thread(target=handle_player, args=(conn, addr), daemon=True).start()

if __name__ == "__main__":
    start()
