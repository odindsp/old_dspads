秒针——加密解密库
=================================

base64.h base64.cpp
---------------------

用于编码、解码经过base64解码、编码后的字符串

openssl_aes.h openssl_aes.cpp
----------------------------

用于加密、解密经过aes解密、加密后的字符串

mz_decode.h mz_decode.cpp
-------------------------

秒针用于加密、解密的方法库（key为hex形式）

main.cpp
--------

调用例子说明

decode
------

可执行的例子程序。

usage:

    * input:
        ** argv[1]: key(hex, 32bit)
        ** argv[2]: plaintext

    * output:
        ** 加密后的字符串
        ** 又解密还原后的字符串
        ** 原始输入信息

