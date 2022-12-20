## 碰到的困難與教訓
- 了解boost::asio的使用方法(與C11的設計風格，call back function(and lambda function)、auto type、smart pointer等等)
    - io_context的運作原理、async跟sync read/write的實作方法等等
- 良好的設計方式，應該是由http server發送狀態碼，再交由cgi program處理
    - 連帶的，直接dup stdin成raw socket，讓cgi直接print到client好像也不是好主意...(不易Debug)
