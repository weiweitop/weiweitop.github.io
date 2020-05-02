---
layout: post
title: "Cloudflare配置考量"
subtitle: "合理薅羊毛:sheep:"
background: '/img/bg-cloudflare.svg'
published: true
imagefolder: "cloudflare"
lastmodify: "2020-04-06"
tags: [搭建博客, CDN]
---

Cloudflare——云闪耀（这个名字好）是全球<abbr title="Content Delivery Network">CDN</abbr>供应商，对免费用户友好:grin:

我们先来看看它有多友好，核心功能包括：

- DDoS attack mitigation：Anycast网络天生自带属性，能让任意一台服务器提供服务，攻击流量自然也就分散了。

- Global Content Delivery Network：激动人心的功能！是的~拨打屏幕下方的电话~你就能免费拥有！中国用户感觉不到是因为Cloudflare不在中国提供服务，换句话说没有<abbr title="Points of Presence">PoP</abbr>在中国。服务由合作伙伴百度云加速提供，还只有Enterprise用户才有:expressionless:

- Support via email：有邮件回就不错了，不过还没联系过。

新文章、新链接肯定是能马上生效的，因为Edge Server里没有Cache就会访问Origin Server。

刚开始觉得Edge Cache TTL设成最低2H很好，那么修改最多2H就能自动生效。但后来仔细想了下，不对啊！对于我们这种偏居一偶的个人博客，每天就零星几个人访问，如果下一个访客在2H之后（大概率间隔还更长），那这个CDN还有什么用？

看来有必要研究下Cloudflare的配置和免费版的Cache行为。

#### 增量式Cache

从效率和成本考虑，我相信Cloudflare是增量Cache的。这里包含两层意思：

1. Cloudflare不会闭着眼睛抓取全站来Cache，又费马达又费电，没有必要。

2. 只有当访问某个页面的时候，HTML和相关资源才会Cache到最近Peer网络的Edge Server，没有必要同步到整个CDN网络，比如某个Edge Server覆盖的地区根本就没人访问你的网站，Cache来做什么呢？

当然，Purge Cache肯定是要通知整个CDN网络的。但不建议Purge Everything，好不容易有个人鬼使神差的访问了你的博客，Cache到了Edge Server，你又去把它清除了，何必呢~何苦呢:smile:

下面设计了一个实验来验证我的想法：

1. 访问[https://weiweitop.fun/cdn-cgi/trace](https://weiweitop.fun/cdn-cgi/trace)，返回的`colo`字段是离Data Center最近的国际机场[<abbr title="International Air Transport Association">IATA</abbr>](https://baike.baidu.com/item/%E6%9C%BA%E5%9C%BA%E4%BB%A3%E7%A0%81/1235719)代码。选择三个国家的代理，分别得到法国CDG、日本NRT、英国AMS。
2. 清除浏览器Cache，Cloudflare Purge Everything，多等几分钟保证生效。
3. 用法国代理访问[首页](https://weiweitop.fun)，清除浏览器Cache，再次访问[首页](https://weiweitop.fun)，确认HTTP Header CF-Cache-Status是HIT。
4. 等待18小时（等不及了），足够广播到整个CDN网络，如果有这个流程的话。
5. 清除浏览器Cache，用日本代理再次确认IATA代码，访问[首页](https://weiweitop.fun)，CF-Cache-Status是MISS
6. 24小时后，用英国代理，还是MISS。

说明了国与国之间不会同步Cache，至于一个国家内的Data Center之间会不会同步，受条件限制，就不知道了。

那么可以推断出，每个地区的第一个访客是感受不得CDN加速的，和直接访问Origin Server差不多。

**以至于Edge Server上的Cache是如此珍贵，Edge Cache TTL设成最大值​并且​控制权​在​自己​手里:v:**

以上只针对免费用户！顶级付费用户的世界我们不懂:wink:

#### Browser Cache TTL

这个配置就涉及到浏览器如何使用Cache，而且主流浏览器之间还有些差异。

地址栏输入网址后回车：

- Win8.1 Firefox 75.0：如果没过期全部用Cache
- Win8.1 Chrome 81.0：如果没过期全部用Cache，只有HTML文档强制发送If-Modified-Since和Cache-Control max-age=0请求，这样就能保证HTML是最新的。我觉得这样还比较合理:+1:
- Safari 13.0.4：HTML文档直接从服务器取，其他资源发送If-Modified-Since请求

F5/刷新按钮/右键Reload/Ctrl+R：

- Win8.1 Firefox 75.0：每个资源发送If-Modified-Since和Cache-Control max-age=0请求，如果没有改变，服务器返回304
- Win8.1 Chrome 81.0：和地址栏输入网址后回车一样
- Safari 13.0.4：和地址栏输入网址后回车一样

Ctrl+F5：

- Win8.1 Firefox 75.0：绕开Cache，每个资源都从服务器取一次
- Win8.1 Chrome 81.0：绕开Cache，每个资源都从服务器取一次
- Safari 13.0.4：（shift+点地址栏旁边的刷新图标）绕开Cache，每个资源都从服务器取一次

比如在浏览器Cache过期前修改了文章和图片，然后在Cloudflare做了Custom Purge，Firefox将看不到改变，F5后才会看到文章和图片的改动，而Chrome将只能看到文章的修改。我觉得大家很少F5，更不要说Ctrl+F5了:laughing:

也就是说我们失去了Cache的控制权。设短了，太多无谓的304；设长了，文章的改动又不能生效。这个只有评估下自己修改的频率了。像我这种基本不更新的博客设置的是12H:joy:

#### Always Online

爬虫程序将会抓取你的网站，在Origin Server挂掉的时候提供有限服务。抓取频率因人而异，免费（Free）用户每隔14天、专业（Pro）用户每隔6天、商业（Business）和企业（Enterprise）用户每隔1天一次。

但只会抓取部分数据，首页前十个链接以及每个深度的第一个链接，最大深度3。

HOME：
- Link 1
  - Link 1-1
    - Link 1-1-1
    - Link 1-1-2 :no_entry:
  - Link 1-2 :no_entry:

- ……
- Link 10
  - Link 10-1
    - Link 10 -1-1
- Link 11 :no_entry:

最佳实践说：

> Do not use Always Online with A Cache Everything Page Rule that configures an Edge Cache TTL lower than the Always Online crawl frequency pertaining to your domain plan type.

比如Edge Cache TTL设成7天，对免费用户来说Always Online14天才抓取一次，当然Edge Cache里的更新鲜，那么开Always Online就没啥意义了。

但Cache Everything就涉及到访问覆盖率的问题，如果网站非常受欢迎，每一个链接都有人访问，那么Cache Everything就相当于全站缓存；如果一个页面从来没有人访问，那肯定是Cache不到的。

Cache Everything是Cache最受欢迎的页面，Always Online是Cache几个固定的页面。对于博客类网站来说(全是GET)，如果Cache完备，在Edge Cache TTL之前，CDN不关心Origin Server挂没挂掉。

不用管最佳实践，激活吧。

{% comment %}
https://imweb.io/topic/5795dcb6fb312541492eda8c HTTP缓存控制小结  Done

https://community.cloudflare.com/t/peering-why-dont-i-reach-the-closest-datacenter-to-me/76479 Done
https://www.cloudflare.com/learning/cdn/what-is-a-cdn/                                         Done
https://www.cloudflare.com/learning/cdn/performance/                                           Done
CDN Glossary                                                           Done
https://www.cloudflare.com/learning/cdn/cdn-ssl-tls-security/          Done
https://www.cloudflare.com/learning/cdn/cdn-load-balance-reliability/  Done



https://blog.cloudflare.com/cloudflare-peering-portal-beta/                 Done
https://blog.cloudflare.com/introducing-cname-flattening-rfc-compliant-cnames-at-a-domains-root/  Done
https://blog.cloudflare.com/edge-side-includes-with-cloudflare-workers/
https://blog.cloudflare.com/cloudflare-ca-encryption-origin/
https://blog.cloudflare.com/introducing-0-rtt/
https://blog.cloudflare.com/staying-on-top-of-tls-attacks/
https://blog.cloudflare.com/dnssec-an-introduction/
https://blog.cloudflare.com/introducing-universal-dnssec/

https://support.cloudflare.com/hc/en-us/articles/200172516 Understanding Cloudflare's CDN                             Done
https://support.cloudflare.com/hc/en-us/articles/216537517 Using Content Security Policy (CSP) with Cloudflare        Done
https://support.cloudflare.com/hc/en-us/articles/200308847 Using cross-origin resource sharing (CORS) with Cloudflare
https://support.cloudflare.com/hc/en-us/articles/360006660072 Configuring DNSSEC                                      Done
https://support.cloudflare.com/hc/en-us/articles/360021111972 Troubleshooting DNSSEC                                  Done

https://support.cloudflare.com/hc/en-us/articles/203118044 Gathering info for troubleshooting          Done
https://support.cloudflare.com/hc/en-us/articles/200168256 Understand Cloudflare Caching Level         Done
https://support.cloudflare.com/hc/en-us/articles/200168276 Understanding Browser Cache TTL             Done
https://support.cloudflare.com/hc/en-us/articles/200168436 Understanding Cloudflare Always Online      Done
https://support.cloudflare.com/hc/en-us/articles/200172556 When does Cloudflare crawl my site          Done
https://support.cloudflare.com/hc/en-us/articles/200168246-Understanding-Cloudflare-Development-Mode   Done
https://support.cloudflare.com/hc/en-us/articles/206776797-Understanding-Query-String-Sort             Done
https://support.cloudflare.com/hc/en-us/articles/202775670 Customizing Cloudflare's cache              Done
https://support.cloudflare.com/hc/en-us/articles/360021023712 Best Practices: Speed up                 Done
https://support.cloudflare.com/hc/en-us/articles/115003206852 Understanding Origin Cache-Control       Done
https://support.cloudflare.com/hc/en-us/articles/218505467 Using ETag                                  Done


https://www.cloudflare.com/dns/dnssec/how-dnssec-works/
https://www.cloudflare.com/learning/
 Learn About DDoS Attacks
 Learn About CDNs
 Learn About DNS https://www.cloudflare.com/learning/dns/dns-security/
 Learn About Web Application Security
 Learn About Performance
 Learn About Serverless
 Learn About SSL
 Learn About Bots
{% endcomment %}