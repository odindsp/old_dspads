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
echo $mapidFile
#if [ "$1" == "amax" -o "$1" == "baidu" -o "$1" == "inmobi" -o "$1" == "letv" -o "$1" == "iqiyi" -o "$1" == "sohu" -o "$1" == "tanx" ]
if [[ "$1" == "AMAX" ]]
then
subkey="adx=2"
elif [[ "$1" == "TANX" ]]
then
subkey="adx=1&"
elif [[ "$1" == "INMOBI" ]]
then
subkey="adx=3"
elif [[ "$1" == "LETV" ]]
then
subkey="adx=5"
#elif [[ "$1" == "ZPLAY" ]]
#then 
#subkey="adx=7"
#elif [[ "$1" == "BAIDU" ]]
#then 
#subkey="adx=8"
#elif [[ "$1" == "GOYOO" ]]
#then 
#subkey="adx=12"
elif [[ "$1" == "IFLYTEK" ]]
then 
subkey="adx=13"
else
echo "unsupported type"
exit 1
fi

year=`expr substr $2 1 4`
mouth=`expr substr $2 5 2`		
day=`expr substr $2 7 2`
emouth=${MOUTHREV[mouth]}
substrde=$day/$emouth/$year

echo $substrde
#grep $substrde /data/log/nginx/odin_$1.log >$2"_dayfile"
rm  -f $outpath"/record_settle"$2"_"$1

grep $subkey /data/log/nginx/backup/odin_settle$2.log >tmp_$1_$2

declare -l subexec 
subexec=settle_"$1"
#执行程序解密价格,生成文件名为tmp_$1_$2_impclk
./$subexec tmp_$1_$2 
mv tmp_$1_$2_impclk tmp_$1_$2

while read T1
do
echo $T1
if [ "$1" == "BAIDU" -o "$1" == "GOYOO" -o "$1" == "ZPLAY" ]
then
grep $substrde":"$T1 "tmp_"$1"_"$2 | grep "mtype=m" > "tmp_"$1"_"$2"_"$T1"_settle"
else
grep $substrde":"$T1 "tmp_"$1"_"$2 |grep "&price=" >"tmp_"$1"_"$2"_"$T1"_settle"
fi
while read MAPID
spent=`grep "$MAPID" "tmp_"$1"_"$2"_"$T1"_settle"|awk -F '&' '{print $3}' |awk '{print $1}' |awk -F'=' '{sum+=$2}END{print sum}'`

echo $2"_"$T1 $MAPID $spent  >> $outpath"/record_settle"$2"_"$1
done < $mapidFile

rm -f "tmp_"$1"_"$2"_"$T1"_settle"
done < tmp_time
rm -f tmp_time
rm -f "tmp_"$1"_"$2

