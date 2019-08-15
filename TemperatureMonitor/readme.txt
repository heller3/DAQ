# temperature monitor howto

1) start temperature monitor from pi

[connect to raspberry]

ssh pi-labo

[address is 128.141.196.54]

[enter a screen session]

screen -S monitor 

[start monitor]

cd Monitor
./readMonitor.py

[leave screen session without closing it]

Ctrl-A Ctrl-D


2) retrieve the temperaure file (.log) 

3) produce plot with gnuplot macro

gnuplot -e "file = 'file.log'" plot.macro


