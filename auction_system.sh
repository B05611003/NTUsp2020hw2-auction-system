#!bin/bash
# author : b05611003
# 2020 system programming hw2

if [ "$#" -ne 2 ]; then
	echo "Illegal number of parameters"
	echo "usage: bash auction_system.sh [n_host] [n_player]"
	exit -1
fi

#------------------------------initial variable--------------------------
ending="-1 -1 -1 -1 -1 -1 -1 -1"
declare -i n_host=${1}
declare -i n_player=${2}
declare -i end_count=${n_host}

declare -a score=( $(for i in $(seq 1 ${n_player}); do echo 0; done)) #score of output
declare -A ranktoscore=([1]=7 [2]=6 [3]=5 [4]=4 [5]=3 [6]=2 [7]=1 [8]=0)
declare -A idtokey
declare -A keytoid

#declare -A pid
players=(1 2 3 4 5 5 6 7 8)
count=0

declare -a combination
#create combination
for (( p1=1; p1<=$n_player; p1++ ));do
	for (( p2=(($p1+1)); p2<=$n_player; p2++ ));do
		for (( p3=(($p2+1)); p3<=$n_player; p3++ ));do
			for (( p4=(($p3+1)); p4<=$n_player; p4++ ));do
				for (( p5=(($p4+1)); p5<=$n_player; p5++ ));do
					for (( p6=(($p5+1)); p6<=$n_player; p6++ ));do
						for (( p7=(($p6+1)); p7<=$n_player; p7++ ));do
							for (( p8=(($p7+1)); p8<=$n_player; p8++ ));do
								combination[++count]="$p1 $p2 $p3 $p4 $p5 $p6 $p7 $p8"								
							done
						done
					done
				done
			done
		done
	done
done

#make fifo
mkfifo $(seq -f "fifo_%g.tmp" 0 $n_host)
read_fifo="fifo_0.tmp"
#open write file des
for i in $(seq 1 ${n_host});do
	exec {fd}<> fifo_${i}.tmp
done





#------------------------------start up hosts--------------------------
start=0    #current write to host
for i in $(seq 1 ${n_host});do
	rand=$RANDOM		#select random key
	idtokey[$i]=$rand	#mapping
	keytoid[$rand]=$i	#mapping
	./host $i $rand 0 &	#startup host
	echo ${combination[++start]} > fifo_${i}.tmp
	#pid[${i}]=$!
done


#------------------------------main loop--------------------------------
readnum=0
exec {fd}<> ${read_fifo}
read_fd=${fd}
while [[ ${readnum} -lt ${count} ]]; do
	#reading stage

	if read -u ${read_fd} key; then
		#assign for host[key]
		#echo "start=${start} count=${count}"
		if [[ ${start} -lt ${count} ]]; then	#if still acution left
			#echo "${combination[${start}]} to fifo_${keytoid[${key}]}.tmp"
			echo ${combination[++start]} > fifo_${keytoid[${key}]}.tmp
		else	#no auction left -> stop host
			#echo "end to fifo_${keytoid[${key}]}.tmp"
			echo ${ending} > fifo_${keytoid[${key}]}.tmp
			#echo "unset host ${keytoid[${key}]}"
			unset keytoid[${key}]
		fi
		#read ranking
		for i in {1..8}; do
			read -u ${read_fd} plyid rank
			score[${plyid}]=$((${score[${plyid}]}+${ranktoscore[${rank}]}))
		done
		((readnum++))
		#echo "finish a round"
	fi
	
	#writing stage
done < ${read_fifo}

for key in ${keytoid[@]}; do
	#echo "end to fifo_${key}.tmp"
   	echo ${ending} > fifo_${key}.tmp
done


#---------------------------done----------------------------------------
for i in $(seq 1 ${n_host});do
	exec {fd}<&-
done

wait
for (( key=1; key<=$n_player; key++ )); do
	echo "${key} ${score[${key}]}"
done
rm -f *.tmp
