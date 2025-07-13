import sys
import serial
import argparse
import time

def main():
    # Парсинг аргументов командной строки
    parser = argparse.ArgumentParser(description='Lora FM')
    parser.add_argument('--freq', type=int, required=True, help='Frequency in Hz')
    parser.add_argument('--read', action='store_true', help='Enable read mode')
    parser.add_argument('--write', action='store_true', help='Enable write mode')
    parser.add_argument('--txpower', type=int,default=2, help='TX power 2..20')
    parser.add_argument('--rate', type=int,default=8000, help='Sample rate 8000,16000')
    parser.add_argument('--port', default='/dev/ttyACM0', help='Serial port (default: /dev/ttyACM0)')
    parser.add_argument('--baud', type=int, default=921600, help='Baud rate (default: 921600)')
    parser.add_argument('--duration', type=int, default=16000, help='RX duration in samples (0-for unlim)')
    parser.add_argument('--input', type=str, help='Input file (stdin if not specified)')
    parser.add_argument('--output', type=str, help='Output file (stdout if not specified)')
    
    args = parser.parse_args()

    if not args.read and not args.write:
        print("Error: You must specify either --read or --write mode", file=sys.stderr)
        return

    if args.read and args.write:
        print("Error: Cannot specify both --read and --write at the same time", file=sys.stderr)
        return

    # Открытие serial порта
    ser = serial.Serial(args.port, args.baud, timeout=100)
    
    if args.read:
        # Режим чтения
        output = open(args.output, 'wb') if args.output else sys.stdout.buffer
        
        try:
            # Отправка команды на чтение
            ser.reset_input_buffer()
            cmd = f"init:{args.freq}:{args.rate}\r\n"
            ser.write(cmd.encode('utf-8'))
            data = ser.readline()
            print(data)
            if data!=b"ok\r\n":
                print("Init failed",file=sys.stderr)
                exit();
            else:
                print("Init ok",file=sys.stderr)    

            cmd = f"read\r\n"
            ser.write(cmd.encode('utf-8'))
            if args.duration!=0:
                print(f"Read {args.duration} samples")
            else:
                print(f"Read samples")
            #print(f"Sending command: {cmd.strip()}", file=sys.stderr)
            
            counter = 0
            try:
                while True:
                    available = ser.in_waiting
                    if available > 0:
                        data = ser.read(available)
                        counter += len(data)
                        output.write(data)
                        if args.duration!=0 and counter>args.duration: 
                            ser.write(b"stop\r\n");    
                            break
                        #print(f"Received {counter} bytes", end='\r', file=sys.stderr, flush=True)
            except KeyboardInterrupt:
                ser.write(b"stop\r\n");    
                print(f"\nFinished. Total received: {counter} bytes", file=sys.stderr)
        finally:
            ser.write(b"stop\r\n");    
            if args.output and output:
                output.close()
    
    elif args.write:
        # Режим записи
        input_src = open(args.input, 'rb') if args.input else sys.stdin.buffer
        
        try:
            ser.reset_input_buffer()
            cmd = f"init:{args.freq}:{args.rate}\r\n"
            ser.write(cmd.encode('utf-8'))
            data = ser.readline()
            print(data)
            if data!=b"ok\r\n":
                print("Init failed",file=sys.stderr)
                exit();
            else:
                print("Init ok",file=sys.stderr)    

            cmd = f"write:{args.txpower}\r\n"
            ser.write(cmd.encode('utf-8'))
            counter = 0
            while True:
                data = input_src.read(8000)
                if not data:
                    break
                while(True):
                    #print("wait ready") 
                    line = ser.readline()
                    if line==b"ready\r\n":
                        #print("get ready") 
                        break
                ser.write(data)
                #ser.flush()
                counter += len(data)
                #print(f"Sent {counter} bytes", end='\r', file=sys.stderr, flush=True)
            ser.write(b"stop\r\n");    
            print(f"\nFinished. Total sent: {counter} bytes", file=sys.stderr)
        finally:
            if args.input and input_src:
                input_src.close()

if __name__ == '__main__':
    main()