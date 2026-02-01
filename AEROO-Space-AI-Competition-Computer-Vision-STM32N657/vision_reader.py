import socket
import datetime

# Vision Data Reader
# Listens on port 5001 and logs received messages

LOG_FILE = r'C:\Users\Suleyman\Desktop\STM32 training\STM32N6-GettingStarted-ObjectDetection-main\vision_data.txt'
UDP_PORT = 5001

def main():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(('0.0.0.0', UDP_PORT))

    print(f'Vision Data Reader started')
    print(f'Listening on port: {UDP_PORT}')
    print(f'Log file: {LOG_FILE}')
    print('Press Ctrl+C to stop\n')

    try:
        while True:
            data, addr = s.recvfrom(4096)
            msg = data.decode('ascii', 'ignore').strip()
            ts = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')

            # Write to log file (only JSON data, no timestamp)
            with open(LOG_FILE, 'a', encoding='utf-8') as f:
                f.write(f'{msg}\n')

            # Print to console (with timestamp for monitoring)
            print(f'{ts} | {addr[0]}:{addr[1]} | {msg}')

    except KeyboardInterrupt:
        print('\n\nStopping Vision reader...')
    finally:
        s.close()
        print('UDP socket closed')

if __name__ == '__main__':
    main()
