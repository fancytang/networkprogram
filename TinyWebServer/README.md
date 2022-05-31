## 项目说明
轻量级Web服务器

## 使用说明
在浏览器输入xxx,打开界面即可使用。

## 文件说明

### /root 
- /html : 网页界面，包括注册、登录、请求文件或视频等。
- /src : 存放网页界面所需要的资源。

### /lock
- 线程同步机制包装类

### /threadpool
- 半同步/半反应堆线程池

### /CGImysql
- CGI和数据库连接池

### /http
- http处理连接类

### /timer
- 定时器处理非活动连接

### /log
- 同步/异步日志

### /test_pressure
- 压力测试

## 测试结果
### connfdLT + listenfdLT
LT+LT+3000
![](./test_pressure/result/LT%2BLT/LT%2BLT%2B3000.PNG)
LT+LT+6000
![](./test_pressure/result/LT%2BLT/LT%2BLT%2B6000.PNG)
LT+LT+9000
![](./test_pressure/result/LT%2BLT/LT%2BLT%2B9000.PNG)
LT+LT+10000
![](./test_pressure/result/LT%2BLT/LT%2BLT%2B10000.PNG)

### connfdLT + listenfdET
LT+ET+3000
![](./test_pressure/result/LT%2BET/LT%2BET%2B3000.PNG)
LT+ET+6000
![](./test_pressure/result/LT%2BET/LT%2BET%2B6000.PNG)
LT+ET+9000
![](./test_pressure/result/LT%2BET/LT%2BET%2B9000.PNG)
LT+ET+10000
![](./test_pressure/result/LT%2BET/LT%2BET%2B10000.PNG)

### connfdET + listenfdLT
ET+LT+3000
![](./test_pressure/result/ET%2BLT/ET%2BLT%2B3000.PNG)
ET+LT+6000
![](./test_pressure/result/ET%2BLT/ET%2BLT%2B6000.PNG)
ET+LT+9000
![](./test_pressure/result/ET%2BLT/ET%2BLT%2B9000.PNG)
ET+LT+10000
![](./test_pressure/result/ET%2BLT/ET%2BLT%2B10000.PNG)

### connfdET + listenfdET
ET+ET+3000
![](./test_pressure/result/ET%2BET/ET%2BET%2B3000.PNG)
ET+ET+6000
![](./test_pressure/result/ET%2BET/ET%2BET%2B6000.PNG)
ET+ET+9000
![](./test_pressure/result/ET%2BET/ET%2BET%2B9000.PNG)
ET+ET+10000
![](./test_pressure/result/ET%2BET/ET%2BET%2B10000.PNG)