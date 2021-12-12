---
layout: posttoc
title: "EffectiveJava2nd Item 68 Code示例"
subtitle: "三个版本的比较"
background: '/img/bg-bagtravel.jpg'
published: false
imagefolder: ""
lastmodify: "2021-09-18"
tags: [Java]
---

#### 原版(CountDownLatch) {#orig}

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

{% highlight java linenos %}

    public static long timeWaitNotify(int concurrency, 
                            final Runnable action) throws InterruptedException{
        final Object ready = new Object();
        final Object start = new Object();
        final Object done = new Object();
        final int[] count = {0, 0};//readyCount doneCount
        
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
                    barrier.await();//the start line, wait other threads stand here
                    action.run();//perform the action
                    barrier.await();//the end line, wait other threads reach here
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