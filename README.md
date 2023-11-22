
陽明交通大學111-1 網路程式設計projects

寫作心得請參見[我的blog](https://laxiflora.github.io/2022/12/21/111-1-交大網路程式設計/)

我的projects有些瑕疵，列表如下:

# project1
自定義的np shell

## 說明

因為程式會初始化PATH環境變數成"bin:."，如果是從github clone下來，請先把project1中bin資料夾內的檔案變成可執行檔，或是在程式中用setenv把PATH改成"/bin"跑linux的指令

### 執行
```
make
./npshell
```

### Example
```bash
% ls
bin  Makefile  npshell  npshell.cpp  README.md  spec  struct.h  test.html  test.txt

% printenv PATH
bin:.

% ls | removetag test.html | cat |2
% ls -al |1
% cat

Test
This is a test program
for ras.

% ls | removetag test.html | cat |2
% ls -al |1
% cat

Test
This is a test program
for ras.

总用量 308
drwxr-xr-x 4 laxiflora laxiflora   4096 11月 22 23:11 .
drwxr-xr-x 7 laxiflora laxiflora   4096 11月 22 22:52 ..
drwxr-xr-x 2 laxiflora laxiflora   4096 11月 22 22:52 bin
-rw-r--r-- 1 laxiflora laxiflora     67 11月 22 22:52 Makefile
-rwxr-xr-x 1 laxiflora laxiflora 266160 11月 22 23:11 npshell
-rw-r--r-- 1 laxiflora laxiflora  10662 11月 22 22:55 npshell.cpp
-rw-r--r-- 1 laxiflora laxiflora    368 11月 22 22:52 README.md
drwxr-xr-x 2 laxiflora laxiflora   4096 11月 22 22:52 spec
-rw-r--r-- 1 laxiflora laxiflora    274 11月 22 22:52 struct.h
-rw-r--r-- 1 laxiflora laxiflora     86 11月 22 22:52 test.html
-rw-r--r-- 1 laxiflora laxiflora     39 11月 22 22:52 test.txt
```
## 心得

測資16(numbered pipe occur in middle , not mixed with ordinary pipe)錯誤，compare出來結果是有一個指令的stdin pipe導向錯誤  
比他還複雜的測資倒是都通過了... 可能剛好卡到某些沒考慮好的問題

# project2

## 說明

一共有三個檔案，對應3種server

- np_simple (Concurrent connection-oriented)
    - 這個server**一次只會處理一個連線，在上個連線離開之前不會accept後續連線**
    - server採concurrent connection-oriented，在accept新request以後會fork出一個新process專門處理request
    - 後續連線雖然無法互動，但因為child process的stdin跟stdout已經導向connection fd，所以可以先輸入內容到stdin，待server一有空即會開始讀取
    - queued request上限為5，超過會捨棄

- np_single_proc (single process conncurrent)
    - 這個server透過polling依次檢查request是否有異動，如果有則會進行處理
    - 處理request跟listen都是使用單一process，故user之間的互動可以透過server處理，不用管IPC
    - 因為是single process，每次要處理一個ip來源的需求前，均需要切換一次環境變數、number pipe table (類似人工轉換process info)與stdio導向的fd，做完一次指令(一次read line)以後再改回stdio (fd 0,1)
    - 如果碰到pipe的對象(讀取訊息的對象/傳輸訊息的對象)尚不存在，則將目標改成/dev/null/ (output is dumped, input will be EOF)

- np_multi_proc (Concurrent connection-oriented)
    - 這個server透過接取request以後fork出新的process來互動
    - 由於需要IPC的互動，會將所有connection的info與mail box放在shared memory裡面
        - 當User A要傳訊息給User B，User A會先將要傳輸的內容塞進User B的message box裡面，並且用SIGUSR去interrupt User B的process，User B的signal handler會去檢查mail box是否為空 / 是否有incoming FIFO request(FIFO是用於pipe commend output，mailbox則用於放message)，若有則打印出來並清空mailbox/創建FIFO檔案並開啟讀取
    - 如果server收到ctrl+c，需依序送出SIGKIL訊號給所有child，以免出現孤兒process


## 功能
- who: 查看目前在線的人
- name: 幫自己重新命名 (name後面一定要打名子，不然會出bug直接被SIGKIL)


```

```


大檔案輸入超過buffer上限時會導致server卡死 (server2,3皆會)，似乎是架構上設計的問題


## project3
console.cgi的連線無法正常結束，不過可以同時執行，在np_golden那方結束輸出以後重新整理也不會導致程式崩潰。應該是有東西卡住io_context了

## project4