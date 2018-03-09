<?php
class Auth {
    
    const SIGNATURE_METHOD = 'HMAC-SHA1';
    
    private $_normalized = array();
    private $_key;
    private $_secret;
    private $_params;
    private $_has_time = 0;
    
    public function __construct($key, $secret)
    {
        $this->_key = $key;
        $this->_secret = $secret;
        
        $this->_params = array(
            'auth_consumer_key' => $this->_key,
            'auth_nonce' => $this->_auth_nonce(),
            'auth_signature_method' => self::SIGNATURE_METHOD,
            'auth_timestamp' => time()-$this->_has_time,
        );
    }
    
    public function setParams($param)
    {
        $normalized = array();
        $this->_params = array_merge($this->_params, $param);
        $this->_params = $this->filterParameters($this->_params);

        ksort($this->_params, SORT_STRING);
        foreach ($this->_params as $key => $value)
        {
            if ($key !== 'auth_signature')
            {
                $normalized[] = urlencode($key) . '=' . urlencode($value);
            }
        }
        $this->_normalized = implode('&', $normalized);
        return $this;
    }

    public function filterParameters($params)
    {
        $filtered = array();

        $query = http_build_query($params);

        if (strlen($query) > 0)
        {
            $params = explode('&', $query);
            foreach ($params as $p)
            {
                @list($name, $value) = explode('=', $p, 2);
                if (!strlen($name))
                {
                    continue;
                }
                $name = urldecode($name);
                $value = urldecode($value);

                if (array_key_exists($name, $filtered))
                {
                    if (is_array($filtered[$name]))
                    {
                        $filtered[$name][] = $value;
                    }
                    else
                    {
                        $filtered[$name] = array($filtered[$name], $value);
                    }
                }
                else
                {
                    $filtered[$name] = $value;
                }
            }
        }

        return $filtered;
    }
    
    public function setMethod($method)
    {
        $this->_method = strtoupper($method);
        return $this;
    }
    
    public function setUrl($uri)
    {
        $this->_uri = strtolower($uri);
        return $this;
    }
    
    private function _auth_nonce()
    {
        return uniqid() . mt_rand();
    }
    
    private function _getBaseString()
    {
        $base_string = $this->_method . '&';
        $base_string .= rawurlencode($this->_uri) . '&';
        $base_string .= rawurlencode($this->_normalized);
        return $base_string;
    }
    
    public function url()
    {
        if ($this->_normalized)
        {
            $signature = $this->generateSignature();
            return $this->_uri .'?'. $this->_normalized .'&auth_signature=' . $signature;
        }
    }
    
    public function queryString()
    {
        if ($this->_normalized)
        {
            $signature = $this->generateSignature();
            return $this->_normalized .'&auth_signature=' . $signature;
        }
    }
    
    public function generateSignature()
    {
        return \base64_encode(hash_hmac('sha1', $this->_getBaseString(), $this->_secret, true));
    }
    
}
function curl($url, $ref_url ='', $data='', $cookie = '', $timeout=40)
{
    $ch = curl_init();
    if ($ref_url != '')
    {
        curl_setopt($ch, CURLOPT_REFERER, $ref_url);
    }
    if ($data)
    {
        curl_setopt($ch, CURLOPT_POST, true);
        curl_setopt($ch, CURLOPT_POSTFIELDS, $data);
    }
    if ($cookie)
    {
        curl_setopt($ch, CURLOPT_COOKIE , $cookie); 
    }

    $header = "application/x-www-form-urlencoded";
    curl_setopt($ch, CURLOPT_URL, $url);
    curl_setopt($ch, CURLOPT_HTTPHEADER, array("Content-type: $header"));
    curl_setopt($ch, CURLOPT_RETURNTRANSFER, true);
    curl_setopt($ch, CURLOPT_CONNECTTIMEOUT, $timeout);
    curl_setopt($ch, CURLOPT_TIMEOUT, $timeout);
    $result = curl_exec($ch);
    $info = curl_getinfo($ch);
    $error = curl_error($ch);
    curl_close($ch);
    return $result;
}


$key = '';
$secret = '';
$apiurl = 'http://api.ad.sohu.com/test/apiauth';


$params = array(
    'aname' => '测试',
    'gender' => '男性',
    'imp' => json_encode(array('http://www.baidu.com/?wd=7','http://www.baidu.com/?wd=8'))
);
$auth = new Auth($key, $secret);
//$string = $auth->setMethod('get')->setUrl($apiurl)->setParams($params)->url(); //get
$qstring = $auth->setMethod('post')->setUrl($apiurl)->setParams($params)->queryString(); //post
//echo '<br/>';
//echo $string;
//$content = curl($string); //get
$content = curl($apiurl, '', $qstring); //post
echo ($content);?>