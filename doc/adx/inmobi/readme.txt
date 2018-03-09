1.inmobi指定的seat = "fc56e2b620874561960767c39427298c"
2.http自定义头没有遵照文档，而是因为历史原因错误的使用，且将错就错使用"application/json; charset=utf-8"
3.did数据无用，因为inmobi直接copy的dpid。主要注意区分一下dpid和ext：os为iOS时，dpid是UDID,而IDFA放在了ext中；os为Android时，dpid是Androidid，但是有的Android请求不携带任何id，我们应该忽略掉这些请求。
4.2015年6月以后的request中，运营商改为字符串表示，如"China Mobile" ,弃用了国际标准码。request主体基于openRTB2.0，但是Device主要基于openRTB2.2
5.2015年9月7日晚，查看inmobi的请求log发现inmobi的流量有问题。在inmobi未告知的情况下，流量从2015年8月6日到2015年8月14日，出现两种货币"USD"和"CNY"都有的情况；从2015年8月28日至今(2015年9月8日)，所有的流量都是"CNY"。inmobi中国团队已确认"CNY"对应的价格就是“人民币”，inmoibi 技术团队回应说结算价格和货币是与我们的response保持一致。
6.inmobi 说: "bid.iurl is used only in our audit tool"，所以response可以不返回iurl
7.inmobi说didmd5和dpidmd5是IMEI
8.2016.3.18，inmobi确定IMEI->didmd5,didsha1,uuid/androidid->dpidmd5,dpidsha1