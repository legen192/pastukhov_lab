set terminal png size 800,600
set output 'timing_graph.png'
set title 'Залежність часу обчислення від кількості потоків'
set xlabel 'Кількість потоків'
set ylabel 'Час виконання (мс)'
set grid
plot 'timing_data.txt' using 1:2 with linespoints lw 2 pt 7 title 'Час виконання'
