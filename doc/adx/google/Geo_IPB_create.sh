#!/bin/sh
#输入参数为要处理的csv文件，输出Geo_IPB.csv

tmpf=Geo_IPB.txt
tmp1=tmp1.csv
tmp2=tmp2.csv
iconv -f gb2312 -t utf-8 $1 > $tmpf
sed -i '1d' $tmpf
sed -i  's/\n$//g'  $tmpf
grep -v "全球" $tmpf > $tmp1
sort -t"," -k 2 $tmp1 > $tmp2
awk -F, '{if($5!="      "){for(i=5;i<75;i+=2) {if($i != "") {print $i "," $1 "," $(i+1) "," $4 "," $2;}}}}'  $tmp2  >$tmpf
iconv -f utf-8 -t gb2312 $tmpf > zzz.txt
rm tmp*.csv -rf


