#!/bin/bash
rm compt.log
rm fail.log
rm score.log
for i in `seq 1 2000`;
do
	./bomberman -debug on -delay 1000 level0.map > out.log
	done=`grep -c "|___/" out.log`
	if [ $done != 1 ]
	then
		more out.log >> fail.log
		printf "Failed : "
	fi
	grep -o "|___/" out.log >> compt.log
	score=$(grep -o [0-9]* out.log | tail -1)
	turns=$(grep -c "NEW\ TURN" out.log | tail -1)
	echo $score >> score.log
	compte=$(grep -c "|___/" compt.log)
	printf "%d / %d | score : %*d | turns : %*d\n" $compte $i 3 $score 4 $turns

done
