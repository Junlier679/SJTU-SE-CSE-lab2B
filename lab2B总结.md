# lab2B：RPC and LockServer

#### Part1

一开始看的时候，让我调用过程改成通过RPC来调用，然后又说RPC帮我写好了？那好像就只要在extent_client里加几行调用RPC端口就行了。似乎很简单……

#### Part2

这个部分让我们完善一下锁的acquire和release操作，我们就在lock_server里维护一下相关的manager和recorder就好

#### Part3

part3则是让我们把写好的锁加到我们的chfs_client中去，以保证我们并行过程中运行正确，其实很简单，就是在每个函数中看一看什么时候acquire什么时候release就好。

结果……碰到了玄学问题，怎么我写完Part3之后，Part1突然过不了了？？会卡在某个文件上，后来查看日志，发现似乎是最开始inode_manager中的write_file函数出问题了，离谱啊……（可能是并行过程放大了这个问题），我也没找到问题出在哪，由于找问题过于烦躁，直接github开找前人的inode_manager/狗头，一顿抄，居然真的过了。。。。。（值得一提的是，在这个过程中，我偶然发现了我的fuse.cc中有个地方写的不对，顺便帮我把symbol link部分的分拿到了……而这时lab1的任务/狗头）

那么现在面临了另外一个更玄学的问题：单独跑Part123都能过，但是跑grade就会在Part3-b卡住。。。。问助教说是WSL系统可能造成这样的问题，那就索性不管了（主要每次卡住都会把我的文件系统弄脏，需要重启电脑，也不想再试了）