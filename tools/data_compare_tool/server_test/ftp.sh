#!/bin/bash

ftp -n <<! 

open 115.182.33.137
user dsp PowerXene123
binary
cd /impclkcheck/
lcd $2
prompt
get $1 $1
close
bye
!

