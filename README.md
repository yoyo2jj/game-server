# game-server

基于 Linux epoll + 线程池 + Protobuf 的轻量级 TCP 游戏服务器，使用 C++17 实现。

## 技术栈

- **网络层**：Linux epoll ET 模式，非阻塞 I/O
- **协议层**：自定义二进制协议（包头+消息类型+Protobuf Body），解决 TCP 粘包
- **序列化**：Google Protobuf 3
- **并发**：自实现线程池（condition_variable + mutex）
- **连接管理**：心跳检测，自动踢掉僵尸连接
- **业务层**：房间系统，支持创建/加入房间，房间满员广播 GameStart

## 架构设计

```
                   ┌──────────────────────────────────────────────┐
                   │                    TcpServer                 │
                   │                                              │
  Client ──TCP──►  │  epoll ET   ──►  handleClientData            │
                   │                        │                     │
                   │                        ▼                     │
                   │                   ThreadPool                 │
                   │                  (4 workers)                 │
                   │                        │                     │
                   │                        ▼                     │
                   │               dispatchMessage                │
                   │              /         |         \           │
                   │        onLogin    onHeartbeat   onRoom       │
                   │                        │                     │
                   │                        ▼                     │
                   │                  RoomManager                 │
                   │                  └── Room[]                  │
                   └──────────────────────────────────────────────┘
```

## 功能特性

- epoll ET 模式处理高并发连接，非阻塞 accept/read 循环
- 自定义二进制协议解决 TCP 粘包问题，每条消息格式：`[4字节总长度][2字节消息类型][Protobuf数据]`
- Protobuf 序列化消息，支持登录请求/响应、心跳、创建房间、加入房间、游戏开始广播
- 每个连接独立 Buffer，数据不够时暂存，凑齐一条完整消息再处理
- 心跳检测：epoll_wait 超时驱动定时器，60 秒无数据自动断开连接
- 线程池：主线程负责 I/O，工作线程处理业务逻辑，网络层与业务层分离
- 房间系统：最多 2 人，房间满员自动广播 GameStart

## 目录结构

```
game-server/
├── CMakeLists.txt
├── proto/
│   └── game.proto          # Protobuf 消息定义
└── src/
    ├── main.cpp
    ├── proto/
    │   ├── game.pb.h        # protoc 生成
    │   └── game.pb.cc
    └── server/
        ├── TcpServer.h/cpp  # 核心服务器，epoll 事件循环
        ├── Connection.h/cpp # 单个连接的状态封装
        ├── Buffer.h/cpp     # 收包缓冲区，处理粘包
        ├── ThreadPool.h/cpp # 线程池
        ├── Room.h/cpp       # 单个房间
        └── RoomManager.h/cpp# 房间管理器
```

## 编译运行

**依赖**

- GCC 7+（需支持 C++17）
- CMake 3.10+
- Protobuf 3.21+

**编译**

```bash
git clone https://github.com/yoyo2jj/game-server.git
cd game-server
mkdir build && cd build
cmake ..
make
```

**运行**

```bash
./game_server
# [Server] Listening on port 8888
# [Server] Running...
```

## 协议设计

每条消息的二进制格式：

```
+-------------------+--------------------+----------------------+
|   Header (4字节)  |  MsgType (2字节)   |   Protobuf Body      |
|  Body总长度(2+N)  |  消息类型编号      |   序列化后的数据     |
+-------------------+--------------------+----------------------+
```

消息类型定义：

| MsgType |                说明                |
|---------|------------------------------------|
|    1    | LoginRequest（登录请求）           |
|    2    | LoginResponse（登录响应）          |
|    3    | Heartbeat（心跳）                  |
|    4    | CreateRoomRequest（创建房间）      |
|    5    | CreateRoomResponse（创建房间响应） |
|    6    | JoinRoomRequest（加入房间）        |
|    7    | JoinRoomResponse（加入房间响应）   |
|    8    | GameStart（游戏开始广播）          |

## 技术亮点

**为什么用 ET 模式而不是 LT？**

ET 只在状态变化时通知一次，必须循环 read/accept 直到 EAGAIN，减少 epoll_wait 调用次数，效率更高。LT 只要缓冲区有数据就持续通知，用起来简单但系统调用次数多。

**粘包如何解决？**

TCP 是字节流协议，没有消息边界。用固定 4 字节包头存消息长度，收到数据先读包头拿到长度，再等够对应字节数才处理，不够就在 Buffer 里等下一次数据到来。

**线程池如何实现？**

任务队列 + condition_variable。主线程把任务 push 进队列后 notify_one，工作线程用 cv.wait(lock, predicate) 阻塞等待，被唤醒后取任务执行。predicate 防止虚假唤醒。

**心跳超时如何触发？**

利用 epoll_wait 的超时参数（设为 10 秒），没有网络事件时自动返回，顺带触发 checkHeartbeat() 扫描所有连接。不需要额外定时器线程，零成本实现定时逻辑。
```
