@echo off
REM ====================================
REM UDP Card Reader Starter
REM ====================================
title UDP Card Reader

cd /d "%~dp0"

python udp_reader.py

pause
