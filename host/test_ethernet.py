import socket

BOARD = ("192.168.1.10", 7)

s = socket.create_connection(BOARD, timeout=3)
print("Connected to", BOARD)

for i in range(5):
    msg = f"hello zynq {i}".encode()
    s.sendall(msg)
    data = s.recv(1024)
    print("OK " if data == msg else "MISMATCH ", data)

s.close()