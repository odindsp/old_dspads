#!/bin/sh

export PATH=$PATH:/bin:/usr/bin:/usr/local/bin;


if [ $# -ne 2 ];
 then echo "please input GroupidMapidFile and date";
 exit;
fi

if [ -e appsid_$2.txt ];
 then
      rm appsid_$2.txt;
fi

path=/data/log/dspads_odin/dsp_impclk/dsp_impclk_$2;

grep -v '^$' $1 | sed -e 's/.$//' > $1.txt

while read groupid mapids
do
  echo "$(grep -R -E "$mapids" $path | grep "appsid:")" >> appsid_$2.txt
done < $1.txt

rm $1.txt
