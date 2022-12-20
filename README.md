
陽明交通大學111-1 網路程式設計projects

寫作心得請參見![我的blog](https://laxiflora.github.io/2022/12/21/111-1-交大網路程式設計/)

我的projects有些瑕疵，列表如下:

## project1
測資16(numbered pipe occur in middle , not mixed with ordinary pipe)錯誤
compare出來結果是有一個指令的stdin pipe導向錯誤
比他還複雜的測資倒是都通過了... 可能剛好卡到某些沒考慮好的問題

## project2
大檔案輸入超過buffer上限時會導致server卡死 (server2,3皆會)
似乎是架構上設計的問題


## project3
console.cgi的連線無法正常結束，不過可以同時執行，在np_golden那方結束輸出以後重新整理也不會導致程式崩潰
應該是有東西卡住io_context了

## project4