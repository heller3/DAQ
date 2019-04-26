# temperature monitor howto

1) start temperature monitor from pi

ssh pi-labo
cd Monitor
python readMonitor.py

start it in screen so you can logout

2) retrieve the temperaure file (.log) 

3) produce plot with gnuplot macro

gnuplot -e "file = 'file.log'" plot.macro


