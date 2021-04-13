

set format x "%R"
set xdata time
set timefmt "%H:%M:%S"
set timestamp

set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically

set xrange["21:07:22":"21:33:00"]
set yrange[85:300]

plot 'incline-mpu.log' using 1:2 
plot 'incline-mpu.log' using 1:2 lt rgb 'red' w l title 'Incline'


set title 'Treadmill Incline Grade'
set ylabel 'grade (degree)'
set xlabel 'time'
set xdata time
set timefmt '%H:%M:%S'
set format x '%H:%M:%S'
set datafile sep ' '
set key top left autotitle columnheader
set grid
set autoscale
set terminal png size 720,650
set output 'taurus_incline_grade.png'
plot 'incline-mpu.log' using 1:2 lt rgb 'red' w l smooth csplines title 'Incline'

