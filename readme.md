# ReADOFAIMacro

一个使用 C++20 编写的 A Dance of Fire and Ice 宏工具项目

可以加载，解析并自动播放谱面

目前键盘输入部分使用Windows API，暂不支持DLC

## 使用:

在Main.cpp中修改vks来分配使用哪些按键，一个按键会分配一个线程，建议分配CPU核心/2个左右

script.play()的第二个参数是一个按键，运行程序后在ADOFAI里打开谱面，在第一个轨道上点击这个按键就会开始播放谱面

构建程序后，运行需要先输入关卡文件路径(例: E:/adofai/level.adofai)

输入后点击前面指定的按键就可以播放了

播放期间可以把鼠标挪到屏幕的左上角(0,0)强制停止播放

## 构建:
`cmake --build build`