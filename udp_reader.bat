@echo off
REM UDP Card Reader - Listens on port 5000 and logs to udp_log.txt
echo Starting UDP card reader...
echo Listening on port 5000
echo Log file: udp_log.txt
echo Press Ctrl+C to stop
echo.

python -u -c "import socket,datetime; path=r'C:\Users\Suleyman\Desktop\STM32 training\STM32N6-GettingStarted-ObjectDetection-main\udp_log.txt'; s=socket.socket(socket.AF_INET,socket.SOCK_DGRAM); s.bind(('0.0.0.0',5000)); print('listening -^>', path); exec(\"while True:\n data,_=s.recvfrom(4096); msg=data.decode('ascii','ignore').strip();\n ts=datetime.datetime.now().strftime('%%Y-%%m-%%d %%H:%%M:%%S');\n with open(path,'a',encoding='utf-8') as f: f.write(f'{ts} {msg}\\n'); print(f'{ts} {msg}')\")"

pause
