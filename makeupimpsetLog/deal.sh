#!/bin/bash
MOUTHREV=("space" "Jan" "Feb" "Mar" "Apr" "May" "Jun" "Jul" "Aug" "Sep" "Oct" "Nov" "Dec")
path="/data/log/nginx/backup/odin_impclk"
outpath="."
#mapidFile="mapidFile"

if [ $# -lt 2 ]
then
echo "input the adx name and select date as 20160101"
exit 1
elif [ $# -gt 2 ]
then
#mapidFile="$3"
str=$3
begin=${str%-*}
end=${str#*-}
else
begin=0
end=23
fi
rm -f tmp_time
echo $begin $end
count=$begin
while [ $count -le "$end" ]
do
printf "%02d\n" $((count++)) >>tmp_time
done


year=`expr substr $2 1 4`
mouth=`expr substr $2 5 2`		
day=`expr substr $2 7 2`
emouth=${MOUTHREV[mouth]}
substrde=$day/$emouth/$year
tty=$day\/$emouth\/$year
mkdir -p $outpath"/"$2
echo $tty
while read T1
do
grep $substrde":"$T1 nginx.log|awk '{print $1","$4","$7}'|sed "s#\[$substrde:#$2,#g" |sed 's/?/,/g' |awk -F ',' '{print $1" "$2","$3" "$5}'  >$outpath"/"$2"/dsp_impclk_"$T1"0000000.log"
done < tmp_time

echo $substrde
