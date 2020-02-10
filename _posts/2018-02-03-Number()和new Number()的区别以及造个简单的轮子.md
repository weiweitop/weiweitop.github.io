---
layout: post
subtitle: "JavaScript"
background: '/img/bg-sciencetech.jpg'
highlighter-theme: pygments_emacs
lastmodify: "2020-02-06"
---

看JavaScript文档的时候注意到了这种用法`var n1 = Number(123);`，冒出的第一个疑问就是和`var n2 = new Number(123);`有什么区别呢？

首先用`typeof`做下探测，n1是`number`而n2是`object`，他们的本质区别就是type不同。

有趣的是`Number`内部肯定知道是怎么调用的它，假设在没有`Number`的情况下，如果我要实现个类似的类应该怎么做呢？

最先想到的就是根据`caller`来区分，但在实验的过程中发现两个问题：
1. 全局调用的时候没有`caller`
1. 就算知道`caller`也无法区分它是`function`调用还是构造对象

所以`caller`这条路就走不通了，既然需要在运行期区分，那么该**真爱**`this`登场了。`this`指向当前构造的对象，那就可以区分是`function`调用还是构造对象了。

新轮子命名为`WeiWeiNumber`，思路理清楚后就剩施工了。为了更接近`Number`的行为，在开工前先用测试数据探测下：
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

首先分析浏览器是不可能把123 auto-box成`Number`对象的，因为两个对象`==`肯定是false的，所以一定是把`Number`对象auto-unbox成原始type（值type）。 查了一下文档对象刚好有个valueOf()方法用来返回这个对象代表的原始值。（后来测试过程中发现valueOf()或toString()实现任一一个方法都能让浏览器返回true）

`string`快速转换成`number`的方法是`"123" * 1 = 123`，但这是语法糖，实际上调用的是`Number("123") * 1`，我们预设Number类是不存在的所以选择计算ASCII码的差值。

下面是实现`WeiWeiNumber`的源码：
{% highlight javascript linenos %}

function WeiWeiNumber(i){
    var primitiveValue = 0;
    
    if(typeof i === "number"){
            primitiveValue = i;
    }else{
        //正则表达式抓取正负符号和数字的文本值
        var regR = /^([\+\-]?)([0-9]+)$/.exec(i);
        if(regR !== null){
            //数字的文本值，相当于Java的group(2)
            var nstr = regR[2];
            var nstrlen = nstr.length;
            
            //callee就是本function，避免hardcode类名
            var nResult = arguments.callee(0);
            for(idx in nstr){
                //通过计算ASCII码的差值转换成数字
                nResult += (nstr[idx].charCodeAt(0) - "0".charCodeAt(0)) 
                              * Math.pow(10, nstrlen - idx -1);
            }
            
            //判断正负值
            if(regR[1] === "-"){
                primitiveValue = -nResult;
            }else{
                primitiveValue = nResult;
            }
        }else{
            primitiveValue = NaN;
        }
    }
    
    if(this instanceof arguments.callee){
        //构造对象
        this.valueOf = function(){
            //为了==判断返回true
            return primitiveValue;
        }
        
        this.toString = function(){
            //为了==判断返回true
            return primitiveValue + '';
        }
        return this;
    }else{
        //invoke as function
        return primitiveValue;
    }
}

{% endhighlight %}