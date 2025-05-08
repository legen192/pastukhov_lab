set terminal png size 800,600
set output "time_vs_threads.png"

set title "Залежність часу обчислень від кількості потоків"
set xlabel "Кількість потоків"
set ylabel "Час виконання (мс)"
set grid

# Встановлюємо стиль ліній та маркерів
set style line 1 lc rgb '#0060ad' lt 1 lw 2 pt 7 ps 1.5

# Встановлюємо діапазон для осі X відповідно до даних
set xrange [0.5:10.5]
set xtics 1

# Побудова графіка
plot "timing.dat" using 1:2 with linespoints ls 1 title "Час виконання"
