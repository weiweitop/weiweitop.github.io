---
layout: posttoc
title: "EffectiveJava2nd Item 68 Code示例"
subtitle: "三个版本的实现"
background: '/img/bg-bagtravel.jpg'
published: false
imagefolder: ""
lastmodify: "2021-09-18"
tags: [Java]
---

#### 原版(CountDownLatch) {#orig}

下面是书上的原版，想要达到的主要目标简单说就是，等所有线程都准备好运行的时候开始计时，然后大家同时运行，等所有线程都运行完成后结束计时。

现实中的例子就是几个同学约到火锅店聚餐，每个人一到火锅店去班长那儿签到：“班长~我到了”。班长一看，全员到齐，于是扯着嗓子：“开干！”。大家一阵胡吃海喝，但每个人吃完后又要到班长那儿报道：“班长~我吃好了”。等吃得最慢的人吃完后，班长又来一嗓子：“收工！”。

{% highlight java linenos %}

    public static long time(Executor executor, int concurrency, 
                        final Runnable action) throws InterruptedException{
        final CountDownLatch ready = new CountDownLatch(concurrency);
        final CountDownLatch start = new CountDownLatch(1);
        final CountDownLatch done = new CountDownLatch(concurrency);
        for(int i = 0; i < concurrency; i++){
            executor.execute(() -> {
                ready.countDown();//tell timer we're ready
                
                try{
                    start.await();//block,wait till peers are ready
                    action.run();//perform the action
                }catch(InterruptedException e){
                    Thread.currentThread().interrupt();
                }finally{
                    done.countDown();//tell timer we're done
                }
            });
        }
        
        ready.await();//block, wait for all workers to be ready
        long startNanos = System.nanoTime();
        start.countDown();//fire the gun
        done.await();//wait for all workers to finish
        return System.nanoTime() - startNanos;
    }

{% endhighlight %}

#### wait-notify版 {#w-n}

下面用作者说的“并发汇编语言”实现。和`CountDownLatch`相比，确实不够优雅，主要体现在下面几个方面：

1. 样板代码太多。但必须要遵从这些基本规则，否则就不能保证正确性。
1. `++`是非原子操作，只能放到同步块以内。
1. 需要自己维护一个准备好和已完成的线程数量。

{% highlight java linenos %}

    public static long timeWaitNotify(int concurrency, 
                            final Runnable action) throws InterruptedException{
        final Object ready = new Object();
        final Object start = new Object();
        final Object done = new Object();
        final int[] count = {0, 0};//readyCount, doneCount
        
        for(int i = 0; i < concurrency; i++){
            new Thread(() -> {
                synchronized(ready){
                    if(++count[0] == concurrency){//NOTE ++ is NOT atomic
                        ready.notifyAll();//tell timer we're ready
                    }
                }
                
                try{
                    synchronized(start){
                        while(count[0] != concurrency){
                            start.wait();//block,wait till peers are ready
                        }
                    }
                    action.run();//perform the action
                }catch(InterruptedException e){
                    Thread.currentThread().interrupt();
                }finally{
                    synchronized(done){
                        if(++count[1] == concurrency){
                            done.notifyAll();//tell timer we're done
                        }
                    }
                }
            }).start();
        }
        
        synchronized(ready){
            while(count[0] != concurrency){
                ready.wait();//block, wait for all workers to be ready
            }
        }
        
        long startNanos = System.nanoTime();
        synchronized(start){
            start.notifyAll();//fire the gun
        }
        
        synchronized(done){
            while(count[1] != concurrency){
                done.wait();//wait for all workers to finish
            }
        }
        
        return System.nanoTime() - startNanos;
    }

{% endhighlight %}

#### CyclicBarrier版 {#cb}

作者说更好的方式是用`CyclicBarrier`实现，查了下API文档和示例代码，确实更好用一点儿。所谓屏障，就是线程在这个地方等待，直到其他线程到齐后再一起向下执行。

它允许所有线程到达屏障点后执行自定义操作，并且不是一次性用后即扔而是可重复使用的屏障，之所以能重复使用是因为抵达屏障点后内部状态被Reset到了初始状态。

在上面火锅的例子中，班长其实是多余的，人员到齐后大家知道自己开吃，不用你喊口令，先把他拿掉。并且屏障是可以重复使用的，开始和结束就用同一个屏障，所以作者说一个`CyclicBarrier`就够了。

那么剩下的事情就简单了，在自定义操作中做时间打卡，在action运行前和运行后设置两处屏障，就达到了我们想要的效果了。

{% highlight java linenos %}

    public static long timeCyclicBarrier(ExecutorService executor, int concurrency, 
                            final Runnable action){
        final List<Long> timeCheckPoint = new ArrayList<>();
        final CyclicBarrier barrier = new CyclicBarrier(
                                    concurrency, 
                                    () -> timeCheckPoint.add(System.nanoTime()));
        List<Callable<String>> tasks = new ArrayList<>();
        for(int i = 0; i < concurrency; i++){
            tasks.add(() -> {
                try{
                    barrier.await();//the start barrier, wait other threads stand here
                    action.run();//perform the action
                    barrier.await();//the end barrier, wait other threads reach here
                }catch(InterruptedException e){
                    Thread.currentThread().interrupt(); 
                }catch(BrokenBarrierException ex){
                    Thread.currentThread().interrupt();
                }
                return null;
            });
        }
        
        try {
            System.out.println(System.currentTimeMillis()+"before invokeAll");
            executor.invokeAll(tasks);//same as invokeAny
            System.out.println(System.currentTimeMillis()+"after invokeAll");
        } catch (Exception e) {
            e.printStackTrace();
        }
        
        return timeCheckPoint.get(1) - timeCheckPoint.get(0);
    }

{% endhighlight %}