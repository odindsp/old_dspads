#include <iostream>
#include "mz_decode.h"
using namespace std;

int main(int argc, char * argv[]){
    if(3 != argc){
        cerr << "usage: " << argv[0] << " hexkey plain_text" << endl;
        return 1;
    }
    string en = mz_encode(argv[1], argv[2]);
    if("" == en){
        cerr << "mz_encode fail." << endl;
        return 1;
    }
    cout << "after encode :" << en<< endl;
    string plain = mz_decode(argv[1], en.c_str());
    if("" == plain){
        cerr << "mz_decode fail." << endl;
        return 1;
    }
    cout << "after decode :" << plain << endl<< endl;
    cout << "input:" << endl;
    cout << "hexkey :" << argv[1] <<endl;
    cout << "plaintext :" << argv[2] << endl;
    return 0;
}
