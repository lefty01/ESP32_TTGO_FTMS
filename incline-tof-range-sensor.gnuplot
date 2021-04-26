

set format x "%R"
set xdata time
set timefmt "%H:%M:%S"
set timestamp

set xtic auto                          # set xtics automatically
set ytic auto                          # set ytics automatically

set xrange["20:41:32":"20:50:54"]
set yrange[85:300]

plot 'incline-tof-range-sensor.log' using 1:2 
plot 'incline-tof-range-sensor.log' using 1:2 lt rgb 'red' w l title 'Incline'


set title 'Treadmill Incline distance'
set ylabel 'distance mm'
set xlabel 'time'
set xdata time
set timefmt '%H:%M:%S'
set format x '%H:%M:%S'
set datafile sep ' '
set key top left autotitle columnheader
set grid
set autoscale
set terminal png size 1024,720
set output 'taurus_incline_dist_tof.png'
plot 'incline-tof-range-sensor.log' using 1:2 lt rgb 'red' w l smooth csplines title 'Incline'

