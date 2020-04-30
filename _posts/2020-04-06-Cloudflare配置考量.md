---
layout: post
title: "Cloudflare配置考量"
subtitle: "合理薅羊毛:sheep:"
background: '/img/bg-cloudflare.svg'
published: false
imagefolder: "cloudflare"
lastmodify: "2020-04-06"
tags: [搭建博客, Cloudflare]
---

Cloudflare——云闪耀（这个名字好）是全球<abbr title="Content Delivery Network">CDN</abbr>供应商，对免费用户友好:joy:

我们先来看看它有多友好，核心功能包括：

- DDoS attack mitigation：Anycast网络天生自带属性，能让任意一台服务器提供服务，攻击流量自然也就分散了。

- Global Content Delivery Network：激动人心的功能！是的~拨打屏幕下方的电话~你就能免费拥有！中国用户感觉不到是因为Cloudflare不在中国提供服务，换句话说没有<abbr title="Points of Presence">PoP</abbr>在中国。服务由合作伙伴百度云加速提供，还只有Enterprise用户才有。

- Support via email：有邮件回就不错了，不过还没联系过。

新文章、新链接肯定是能马上生效的，因为Edge Server里没有Cache就会访问Origin Server。

刚开始觉得Edge Cache TTL设成最低2H很好，那么修改最多2H就能自动生效。但后来仔细想了下，不对啊！对于我们这种偏居一偶的个人博客，每天就零星几个人访问，如果下一个访客在2H之后（大概率间隔还更长），那这个CDN还有什么用？

看来有必要研究下Cloudflare的配置和免费版的Cache行为。

#### 增量式Cache

从效率和成本考虑，我相信Cloudflare是增量Cache的。这里包含两层意思：

1. Cloudflare不会闭着眼睛抓取全站来Cache，又费马达又费电，没有必要。

2. 只有当访问某个页面的时候，HTML和相关资源才会Cache到最近Peer网络的Edge Server，没有必要同步到整个CDN网络，比如某个Edge Server覆盖的地区根本就没人访问你的网站，Cache来做什么呢？

当然，Purge Cache肯定是要通知整个CDN网络的。但不建议Purge Everything，好不容易有个人鬼使神差的访问了你的博客，Cache到了Edge Server，你又去把它清除了，何必呢，何苦呢:smile:

下面设计了一个实验来验证我的想法：

1. 访问[https://weiweitop.fun/cdn-cgi/trace](https://weiweitop.fun/cdn-cgi/trace)，返回的`colo`字段是离Data Center最近的国际机场[<abbr title="International Air Transport Association">IATA</abbr>](https://baike.baidu.com/item/%E6%9C%BA%E5%9C%BA%E4%BB%A3%E7%A0%81/1235719)代码。选择三个国家的代理，分别得到法国CDG、日本NRT、英国AMS。
2. 清除浏览器Cache，Cloudflare Purge Everything，多等几分钟保证生效。
3. 用法国代理访问[首页](https://weiweitop.fun)，清除浏览器Cache，再次访问[首页](https://weiweitop.fun)，确认HTTP Header CF-Cache-Status是HIT
4. 等待18小时（等不及了），足够广播到整个CDN网络，如果有这个流程的话。
5. 清除浏览器Cache，用日本代理再次确认IATA代码，访问[首页](https://weiweitop.fun)，CF-Cache-Status是MISS
6. 24小时后，用英国代理，还是MISS

说明了国与国之间不会同步Cache，至于一个国家内的Data Center之间会不会同步，受条件限制，就不确定了。

那么可以推断出，每个地区的第一个访客是感受不得CDN加速的，和直接访问Origin Server差不多。

Edge Server上的Cache是如此珍贵，Edge Cache TTL设成最大值:v:

**以上只针对免费用户！顶级付费用户的世界我不懂！**

#### Browser Cache TTL

Browser Cache TTL的设置就涉及到浏览器的行为

Firefox/Chrome/Safari行为的差异


#### Always Online

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