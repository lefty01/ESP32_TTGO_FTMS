

set format x "%R"
set xdata time
set timefmt "%H:%M:%S"
set timestamp

set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically

#set xrange["21:07:22":"21:33:00"]
set xrange [*:]

set yrange[0:8]

plot 'incline-mpu.log' using 1:2 
plot 'incline-mpu.log' using 1:2 lt rgb 'red' w l title 'Incline'


set title 'Treadmill Incline Angle'
set ylabel 'angle (degree)'
set xlabel 'time'
set xdata time
set timefmt '%H:%M:%S'
set format x '%H:%M:%S'
set datafile sep ' '
set key top left autotitle columnheader
set grid
#set autoscale
set terminal png size 1024,720
set output 'taurus_incline_angle.png'
plot 'incline-mpu.log' using 1:2 lt rgb 'red' w l smooth csplines title 'Incline'

