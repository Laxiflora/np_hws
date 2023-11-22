
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

## 說明

這個project實作了http server與CGI_server
- http server做為對外窗口，會接受來自http protocol的request，處理完GET需求以後，將對應的服務轉交給CGI server執行
- CGI server，則處在執行待命狀態，當收到來自http server的命令時，接受請求並執行程式 (我在project中將CGI server的stdio均改成了socket fd)

這個project需要用到[Boost.Asio](https://www.boost.org/users/history/version_1_83_0.html)，版本1.83.0可以執行
下載Boost.Asio以後移動到下載位置(預設是這個repo的根目錄)，開始安裝，步驟如下
```
tar -xvf boost_1_83_0.tar.gz
cd boost_1_83_0
./bootstrap.sh
./b2
sudo ./b2 install # 安裝boost到system
```
安裝完成以後，根據執行http server的系統
```
# windows
make part2

# linux
make part1
```
編譯完以後就可以執行
```
./http_server [port]
```
打開瀏覽器，輸入URL "http://localhost:[port]/panel.cgi" (如果跑在其他server就把localhost改掉)
選擇要跑動的伺服器(其他都是工作站，現已不可用，選最後的local host)，在那個伺服器中執行 `./np_single_golden [port]`，並在panel.cgi中填入np_single_golden的port

按下run，則程式開始執行


## 功能
透過http server指示CGI server執行project 2 的np_simple_golden (Concurrent connection-oriented)

## 心得
- [BUG] console.cgi的連線無法正常結束，不過可以同時執行，在np_golden那方結束輸出以後重新整理也不會導致程式崩潰。應該是有東西卡住io_context了
- 根據設計規範，http狀態碼應該經由http server回傳，而非CGI server
- 另外也不確定直接將CGI的stdio改成socket fd是不是好事(前面兩個project中毒)，好處是CGI exec的program其stdout不需要做任何更動，不過至少在這次project中這樣搞讓整個debug流程更複雜了 

## project4