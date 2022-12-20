# README #

This README would normally document whatever steps are necessary to get your application up and running.

## 碰到的困難與教訓

- PipeFd應該要能關就關，但如果已經關過這個fd，不要本著保險起見的態度在別的地方再關一次!
    - 很有可能會把兩次關閉之間有開啟的fd意外關閉(linux預設將fd分配到最低的available id)，造成意想不到的後果 (而且有夠難debug = =)

- Fd Table 的生存範圍是行程內部，不同process之間不互通，fork以後不該期待他們會繼續共享，需要把user pid跟fd記錄在share memory

- share memory是切割一塊可用的記憶體空間給大家共用，換言之，要確保所有想共用的資料都保存在裡面!
    - malloc只會讓資料保存在heap，在fork之後的資料是不互用的，而且因為malloc會把資料丟到heap，理論上位置會偏離shared memory，對於shared memory，建議是設好以後就不要再dynamic allocate，除非重新劃分
        - The memory you allocate to a pointer using malloc() is private to that process. So, when you try to access the pointer in another process(other than the process which malloced it) you are likely going to access an invalid memory page or a memory page mapped in another process address space. So, you are likely to get a segfault.
        - https://stackoverflow.com/questions/14558443/c-shared-memory-dynamic-array-inside-shared-struct



        https://stackoverflow.com/questions/16400820/how-to-use-posix-semaphores-on-forked-processes-in-c


        https://stackoverflow.com/questions/24130990/i-used-waitstatus-and-the-value-of-status-is-256-why

- 不要在signal handler裡面跑任何non reentrant的function，比如printf
    - [安全的function list](https://man7.org/linux/man-pages/man7/signal-safety.7.html)
    - 可以用strcpy，不可用sprintf