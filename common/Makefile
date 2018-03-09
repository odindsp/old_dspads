CC = g++

all:
	$(CC) ../common/tanx/DecodePrice.cpp -fPIC -shared -o libtanx.so -lcrypto -O2
	${CC} ../common/amax/decrypter.cc -fPIC -shared -o libamax.so -lcrypto -O2
	${CC} ../common/inmobi/decode.cpp -fPIC -shared -o libinmobi.so -lcrypto -O2
	${CC} ../common/letv/decode.cpp ../common/base64.c ../common/algo_aes.cpp -fPIC -shared -o libletv.so -lcrypto -O2
	${CC} ../common/iwifi/decode.cpp -fPIC -shared -o libiwifi.so -lcrypto -O2
	${CC} ../common/toutiao/DecodePrice.cpp -fPIC -shared -o libtoutiao.so -lcrypto -O2
	${CC} ../common/sohu/priceDecrypt.cpp -fPIC -shared -o libsohu.so -lcrypto -O2
	${CC} ../common/baidu/decode.cpp -fPIC -shared -o libbaidu.so -lcrypto -lssl -O2
	${CC} ../common/goyoo/decode.cpp -fPIC -shared -o libgoyoo.so -lcrypto -lssl -O2
	gcc	  ../common/zplay/decode.c ../common/zplay/hmac_sha1.c ../common/zplay/hex.c ../common/zplay/base64.c -fPIC -shared -o libzplay.so --std=c99 -O2
	${CC} ../common/iqiyi/decode.cpp ../common/iqiyi/settlement.pb.cc -fPIC -shared -o libiqiyi.so -lssl -lcrypto -lprotobuf -O2
	${CC} ../common/iflytek/decode.cpp ../common/iflytek/base64.c -fPIC -shared -o libiflytek.so -lssl -lcrypto -O2
	${CC} ../common/gdt/decode.cpp ../common/gdt/base64.c -fPIC -shared -o libgdt.so -lssl -lcrypto -O2
	${CC} ../common/adview/decrypter.cc -fPIC -shared -o libadview.so -lcrypto -O2
	${CC} ../common/google/decrypter.cc -fPIC -shared -o libgoogle.so -lcrypto -O2
	${CC} ../common/lefee/decrypter.cc -fPIC -shared -o liblefee.so -lcrypto -O2
	gcc   ../common/autohome/adx_encrypt.c  -o libautohome.so -fPIC -shared -lcrypto -lssl -O2 -std=c99
	${CC} ../common/baiduvideos/decode.cpp -fPIC -shared -o libbaiduvideo.so -lcrypto -O2
	${CC} ../common/momo/decode.cpp ../common/momo/base64.c -fPIC -shared -o libmomo.so -lssl -lcrypto -O2
	${CC} ../common/lingji/mz_decode.cpp ../common/lingji/base64.cpp ../common/lingji/openssl_aes.cpp -fPIC -shared -o liblingji.so -lssl -lcrypto -O2
install:
	mv *.so /usr/local/lib64/dspads_odin/

.PHONY: clean
clean:
	-rm $(OBJECTS)

#end of file

