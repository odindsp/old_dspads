<?php
/*-------------------------example-----------------------------*/
$encKey = "7183e751829e457488a8f9e13cdb1292";
$sigKey = "b4056881520040a0af16fdccb99d68bb";
$PriceDec = new PriceDec($encKey, $sigKey);
$price = $PriceDec->decode("JR9YVr-YW3HcCETOOcw5nA");
var_dump($price);
/*-------------------------example-----------------------------*/

class PriceDec
{

    protected $encKey;

    protected $sigKey;

    public function __construct($encKey, $sigKey)
    {
        $this->encKey = hex2bin($encKey);
        $this->sigKey = hex2bin($sigKey);
    }

    public function decode($base64Str)
    {
        $data = $this->base64urlDecode($base64Str);
        if (!$data || strlen($data) != 16) {
            return false;
        }
        
        $timeMsStamp = substr($data, 0, 4);
        $encPrice = substr($data, 4, 8);
        $sig = substr($data, 12, 4);
        
        $pad = hex2bin(hash_hmac('sha1', $timeMsStamp, $this->encKey));
        $decPrice = $this->xorBytes($pad, $encPrice, 8);
        $price = unpack('d', $decPrice);
        
        if (!isset($price[1])) {
            return false;
        }
        
        $osig = hex2bin(hash_hmac('sha1', $decPrice . $timeMsStamp, $this->sigKey));
        if ($sig != substr($osig, 0, strlen($sig))) {
            return false;
        }
        
        return $price[1];
    }

    protected function xorBytes($s1, $s2, $length)
    {
        $result = "";
        for ($i = 0; $i < 8; ++ $i) {
            $result .= chr(ord($s1[$i]) ^ ord($s2[$i]));
        }
        
        return $result;
    }

    protected function base64urlDecode($base64Str)
    {
        $base64Str = str_replace(array('-','_'), array('+','/'), $base64Str);
        $base64Len = strlen($base64Str);
        $pad = 4 - ($base64Len % 4);
        $pad < 4 && $base64Str = str_pad($base64Str, $base64Len + $pad, "=");
        
        return base64_decode($base64Str);
    }
}