/****************************************************************/  
/* 802.11i HMAC-SHA-1 Test Code                                 */  
/* Copyright (c) 2002, David Johnston                           */  
/* Author: David Johnston                                       */  
/* Email (home): dj@deadhat.com                                 */  
/* Email (general): david.johnston@ieee.org                     */  
/* Version 0.1                                                  */  
/* 
    This program is free software: you can redistribute it and/or modify 
    it under the terms of the GNU General Public License as published by 
    the Free Software Foundation, either version 2 of the License, or 
    (at your option) any later version. 
 
    This program is distributed in the hope that it will be useful, 
    but WITHOUT ANY WARRANTY; without even the implied warranty of 
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
    GNU General Public License for more details. 
 
    You should have received a copy of the GNU General Public License 
    along with this program.  If not, see <http://www.gnu.org/licenses/>. 
*/  
/*                                                              */  
/* This code implements the NIST HMAC-SHA-1 algorithm as used   */  
/* the IEEE 802.11i security spec.                              */  
/*                                                              */  
/* Supported message length is limited to 4096 characters       */  
/* ToDo:                                                        */  
/*   Sort out endian tolerance. Currently little endian.        */  
/****************************************************************/  
#ifndef HMAC_HAC1_H
#define HMAC_HAC1_H

#include <stdlib.h>  
#include <stdio.h>  
  
extern unsigned long int ft(  
                    int t,  
                    unsigned long int x,  
                    unsigned long int y,  
                    unsigned long int z  
                    );  
  
extern int get_testcase(   int test_case,  
                    unsigned char *plaintext,  
                    unsigned char *key,  
                    int *key_length_ptr);  
  
  
extern void sha1   (  
            unsigned char *message,  
            int message_length,  
            unsigned char *digest  
            );  
  
  
/****************************************/  
/* sha1()                               */  
/* Performs the NIST SHA-1 algorithm    */  
/****************************************/  
  
extern unsigned long int ft(  
                    int t,  
                    unsigned long int x,  
                    unsigned long int y,  
                    unsigned long int z  
                    )  ;
  
extern unsigned long int k(int t)  ;
  
extern unsigned long int rotr(int bits, unsigned long int a)  ;
  
extern unsigned long int rotl(int bits, unsigned long int a) ;
  
extern void sha1(unsigned char *message,  
            int message_length,  
            unsigned char *digest);
  
/******************************************************/  
/* hmac-sha1()                                        */  
/* Performs the hmac-sha1 keyed secure hash algorithm */  
/******************************************************/  
  
extern void hmac_sha1(unsigned char *key,  
                int key_length,  
                unsigned char *data,  
                int data_length,  
                unsigned char *digest)  ;

#endif