## 碰到的困難與教訓
- 注意甚麼時候會需要處理sock4a中的HOST_NAME (搞懂0.0.0.1的意思)
- 因為protocol的傳輸都會是binary形式，要注意default data type有幾個byte
    - 留意sock4中的data transfer都是用unsigned char傳輸，如果用char[]去接，會有負號需要處理
    - 如port等等需轉成int type(或使用resolver處理)但分為2 byte儲存的數據，處理上會有一些挑戰
- 如果bind request遲遲收不到，可以留意一下是不是被自己的防火牆擋住了(我被我的McAFee搞到好久= =)



## 參考資料
[connecting-to-ip-0-0-0-0-succeeds-how-why](https://unix.stackexchange.com/questions/419880/connecting-to-ip-0-0-0-0-succeeds-how-why)