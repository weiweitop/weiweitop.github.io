---
layout: post
title: "Cloudflare配置考量"
subtitle: "合理薅羊毛"
background: '/img/bg-cloudflare.svg'
published: false
imagefolder: "cloudflare"
lastmodify: "2020-04-06"
tags: [搭建博客, Cloudflare]
---

Cloudflare云闪耀（这个名字好）是个全球<abbr title="Content Delivery Network">CDN</abbr>供应商，对免费用户友好:joy:

我们先来看看它有多友好，核心功能包括：

- DDoS attack mitigation：Anycast网络天生自带属性，能让任意一台服务器提供服务，攻击流量自然也就分散了。

- Global Content Delivery Network：激动人心的功能！是的~拨打屏幕下方的电话~你就能免费拥有！中国用户感觉不到是因为Cloudflare不在中国提供服务，换句话说没有<abbr title="Points of Presence">PoP</abbr>在中国。服务由合作伙伴百度云加速提供，还只有Enterprise用户才有。

- Support via email：有邮件回就不错了，不过还没联系过。

新文章、新链接肯定是能马上生效的，因为Edge Server里没有Cache就会访问Origin Server。

刚开始觉得Edge Cache TTL设成最低2H很好，修改最多2H就能自动生效。但后来仔细想了下，不对啊！对于偏居一偶的个人博客，每天就零星几个人访问，如果下一个访客在2H之后（大概率间隔还更长），那这个CDN还有什么用？

看来有必要研究下Cloudflare的配置和行为

#### A


#### Firefox/Chrome/Safari行为的差异

Browser Cache TTL的设置就涉及到浏览器的行为

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