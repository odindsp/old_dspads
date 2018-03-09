export PATH=$PATH:/bin:/usr/bin:/usr/local/bin
#!/bin/sh
#cd /data/log/dspads_odin/dsp_impclk/dsp_impclk_20151119
#path="/data/log/dspads_odin/dsp_impclk/dsp_impclk_20151119/*";
#rm count_$(date -d last-day +%Y%m%d).txt
#echo  $(date -d last-day +%Y%m%d);

#if [ $# -ne 1 ]
#then
#  echo "input filename";
#  exit 1
#fi
if [ $# -ne 2 ];
 then echo "please input GroupidMapidFile and date";
 exit;
fi

if [ -e count_$2.txt ];
 then
      rm count_$2.txt;
fi
path=/data/log/dspads_odin/dsp_impclk/dsp_impclk_$2;


grep -v '^$' $1 | sed -e 's/.$//' > $1.txt


if [ ! -e $path ]
 then echo "log file  $path not exit";
 exit;
fi

echo "groupid						imp       		clk" > count_$2.txt
while read groupid mapids
do
m=`grep -R -E "$mapids" $path|grep "mtype:m"|wc -l`;
c=`grep -R -E "$mapids" $path|grep "mtype:c"|wc -l`;
echo "$groupid		$m     		$c" >> count_$2.txt
done < $1.txt

rm -f $1.txt

