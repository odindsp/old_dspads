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

if [ -e baseInfoCount_$2.txt ];
 then
      rm baseInfoCount_$2.txt;
fi
path=/data/log/dspads_odin/dsp_impclk/dsp_impclk_$2;


if [ ! -e $path ]
 then echo "log file  $path not exit";
 exit;
fi

grep -v '^$' $1 | sed -e 's/.$//' > $1.txt

while read groupid mapids
do
  echo "$(grep -R -E "$mapids" $path)" >>baseInfoCount_$2.txt
done < $1.txt


grep "winprice" baseInfoCount_$2.txt > winprice
awk -F ',' '{print$10}' winprice > device1.txt
awk -F ':' '{print$2}' device1.txt > device.txt
sed -i '/winprice/d' baseInfoCount_$2.txt
awk -F ',' '{print$9}' baseInfoCount_$2.txt > device2.txt
awk -F ':' '{print$2}' device2.txt >>device.txt

sort device.txt | uniq -c > deviceid.txt
rm winprice;
rm device1.txt;
rm device.txt;
rm device2.txt;
rm $1.txt
