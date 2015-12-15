#
# sample Gnuplot command file for iperf3 results
set term x11
#set term png 
#set term postscript landscape color
set key width -12

# iperf3 data fields
#start bytes bits_per_second retransmits snd_cwnd

set output "iperf3.png"
#set output "iperf3.eps"

#set nokey

set grid xtics
set grid ytics
set grid linewidth 1
set title "TCP performance: 40G to 10G host"
set xlabel "time (seconds)"
set ylabel "Bandwidth (Gbits/second)"
set xrange [0:60] 
set yrange [0:15] 
set ytics nomirror
set y2tics
set y2range [0:2500] 
# dont plot when retransmits = 0
set datafile missing '0'
set pointsize 1.6

plot "40Gto10G.old.dat" using 1:3 title '3.10 kernel' with linespoints lw 3 pt 5, \
	 "40Gto10G.new.dat" using 1:3 title '4.2 kernel' with linespoints lw 3 pt 7, \
 	 "40Gto10G.old.dat" using 1:4 title 'retransmits' with points pt 7 axes x1y2

#plot "iperf3.old.dat" using 1:3 title '3.10 kernel' with linespoints lw 3 pt 5, \
#	 "iperf3.new.dat" using 1:3 title '4.2 kernel' with linespoints lw 3 pt 7

