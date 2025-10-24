import socket

def recv_smtp(sock, expected_code=None):
    response = ""
    while True:
        chunk = sock.recv(1024)
        if not chunk:
            break  # Connection closed by server
        chunk = chunk.decode()
        response += chunk
        print(chunk, end="")
        lines = response.splitlines()
        if lines and len(lines[-1]) >= 4 and lines[-1][3] == " ":
            if expected_code and not lines[-1].startswith(str(expected_code)):
                print("Unexpected response!")
            return response

with socket.create_connection(("localhost", 2525)) as sock:
    recv_smtp(sock, 220)
    sock.sendall(b"EHLO test.com\r\n")
    recv_smtp(sock, 250)
    sock.sendall(b"MAIL FROM: <test@example.com>\r\n")
    recv_smtp(sock, 250)
    sock.sendall(b"RCPT TO: <recipient@example.com>\r\n")
    recv_smtp(sock, 250)
    sock.sendall(b"DATA\r\n")
    recv_smtp(sock, 354)
    sock.sendall(b"Subject: Test from script\r\n\r\nThis is a test message sent from a script!\r\n.\r\n")
    recv_smtp(sock, 250)
    sock.sendall(b"QUIT\r\n")
    recv_smtp(sock, 221)
    sock.close()