import socket
import threading
import sys

def receive_messages(client_socket):
    """Constantly listens for broadcasts from the banker."""
    while True:
        try:
            message = client_socket.recv(1024).decode()
            if message:
                # \r clears the current line so the update doesn't mess up your typing
                sys.stdout.write('\r' + message + '\nYour Bet: ')
                sys.stdout.flush()
            else:
                break
        except:
            print("\n[ERROR] Connection lost.")
            break

def start_player():
    banker_ip = input("Banker IP: ")
    name = input("Your Name: ")
    
    client = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    try:
        client.connect((banker_ip, 65432))
        client.send(name.encode()) # Send name immediately after connecting
        
        # Start a thread to listen for broadcasts in the background
        thread = threading.Thread(target=receive_messages, args=(client,), daemon=True)
        thread.start()
        
        while True:
            bet = input("Your Bet: ")
            if bet.lower() == 'q': break
            client.send(bet.encode())
            
    except Exception as e:
        print(f"Error: {e}")
    finally:
        client.close()

if __name__ == "__main__":
    start_player()