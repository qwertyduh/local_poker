import socket
import threading

def get_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    ip = s.getsockname()[0]
    s.close()
    return ip

HOST = get_ip()
PORT = 65432
clients = {} # Stores {socket: name}
total_pot = 0

def broadcast(message):
    """Sends a message to every connected player."""
    for client_socket in clients:
        try:
            client_socket.send(message.encode())
        except:
            # Remove broken connections
            client_socket.close()

def handle_player(conn, addr):
    global total_pot
    try:
        # First message from player is always their name
        name = conn.recv(1024).decode()
        clients[conn] = name
        
        welcome_msg = f"\n>>> {name} joined the table! Current Pot: ${total_pot}"
        print(welcome_msg)
        broadcast(welcome_msg)

        while True:
            msg = conn.recv(1024).decode()
            if not msg: break
            
            if msg.isdigit():
                bet = int(msg)
                total_pot += bet
                update = f"\n[UPDATE] {name} bet ${bet}. Total Pot is now: ${total_pot}"
                print(update)
                broadcast(update)
    except:
        pass
    finally:
        if conn in clients:
            leave_msg = f"\n>>> {clients[conn]} left the table."
            print(leave_msg)
            del clients[conn]
            broadcast(leave_msg)
        conn.close()

def start():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.bind((HOST, PORT))
    server.listen()
    print(f"[BANKER ACTIVE] IP: {HOST} | Waiting for players...")
    while True:
        conn, addr = server.accept()
        thread = threading.Thread(target=handle_player, args=(conn, addr))
        thread.start()

if __name__ == "__main__":
    start()