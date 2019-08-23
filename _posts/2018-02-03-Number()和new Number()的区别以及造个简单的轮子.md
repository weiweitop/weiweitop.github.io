---
layout: post
background: '/img/posts/科技预言.jpg'
#highlighter-theme: rouge_monokai
---

看JavaScript文档的时候注意到了这种用法`var n1 = Number(123);`，冒出的第一个疑问就是和`var n2 = new Number(123);`有什么区别呢？

首先用typeof做下探测，n1是number而n2是object，他们的本质区别就是type不同。

那么有趣的问题来了，Number内部肯定知道是怎么调用的它。假设在没有Number的情况下，如果我要实现个类似的类应该怎么做呢？

最先想到的就是根据caller来区分，但在实验的过程中发现两个问题：
1. 全局调用的时候没有caller
2. 就算知道caller也无法区分它是function调用还是构造对象

所以caller这条路就走不通了，既然需要在运行期区分，那么该**真爱**`this`登场了。`this`指向当前构造的对象，我就可以区分是function调用还是构造对象了。

我的新轮子命名为WeiWeiNumber，思路理清楚后就剩施工了。为了更接近Number的行为，在开工前先用测试数据探测下：
{% highlight javascript linenos %}
console.log(Number(123));      //123
console.log(Number(+123));     //123
console.log(Number(-123));     //-123
console.log(Number("123"));    //123
console.log(Number("+123"));   //123
console.log(Number("-123"));   //-123
console.log(Number("abc123")); //NaN
console.log(Number(NaN));      //NaN


console.log(new Number(123)); //save as above except type
console.log(new Number(+123));
console.log(new Number(-123));
console.log(new Number("123"));
console.log(new Number("+123"));
console.log(new Number("-123"));
console.log(new Number("abc123"));
console.log(new Number(NaN));
{% endhighlight %}

在测试过程发现`123 == new Number('123')`是返回`true`的，但我们的`123 == new WeiWeiNumber('123')`却返回`false`，难道浏览器不给`WeiWeiNumber`国民待遇？

首先浏览器是不可能把123 auto-box成Number对象的，因为两个对象==是false的，所以肯定是把Number对象auto-unbox成原始type（值type）。 查了一下文档对象刚好有个valueOf()方法用来返回这个对象代表的原始值。（后来测试过程中发现valueOf()或toString()实现任一一个方法都能让浏览器返回true）

string快速转换成number的方法是 "123" * 1 = 123，但这是语法糖，实际上调用的是 Number("123") * 1，我们预设Number类是不存在的所以选择计算ASCII码的差值。

下面是实现WeiWeiNumber的源码：
```javascript
function go(){
  alert(123);
}
```

{% highlight javascript linenos %}
function go(){
  alert(123);
}
{% endhighlight %}