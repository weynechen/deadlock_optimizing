# FreeRTOS-Sim

这是一个基于Posix的FreeRTOS模拟器示例项目，用于在Linux系统上模拟运行FreeRTOS。当前示例实现了一个死锁演示程序，以展示多任务环境下互斥量使用不当导致的死锁问题，并加入了死锁检测机制。

## 运行环境

- **操作系统**：Linux（已在Ubuntu系统上测试）
- **编译器**：gcc-11（支持32位编译）
- **依赖库**：pthread（用于线程支持）

## 系统要求

1. 32位支持环境（项目使用-m32编译标志）
2. gcc-11编译器
3. 支持pthread库

## 安装依赖

在Ubuntu系统上，可以通过以下命令安装所需依赖：

```bash
# 安装GCC 11
sudo apt-get update
sudo apt-get install gcc-11 g++-11

# 安装32位支持库
sudo apt-get install gcc-11-multilib g++-11-multilib libc6-dev-i386
```

## 项目结构

```
.
├── Source/                  - FreeRTOS源代码
├── Project/                 - 项目代码
│   ├── main.c               - 主程序入口
│   ├── deadlock_demo.c      - 死锁演示实现
│   ├── deadlock_demo.h      - 死锁演示头文件
│   ├── deadlock_detection.c - 死锁检测实现
│   ├── deadlock_detection.h - 死锁检测头文件
│   └── FreeRTOSConfig.h     - FreeRTOS配置文件
├── doc/                     - 项目文档
├── Makefile                 - 项目构建文件
└── obj/                     - 编译生成的目标文件目录
```

## 构建和运行

### 构建项目

运行以下命令构建项目：

```bash
make
```

成功构建后将生成`FreeRTOS-DeadlockDemo`可执行文件。

### 运行演示程序

运行以下命令启动死锁演示：

```bash
make run
```

或直接运行可执行文件：

```bash
./FreeRTOS-DeadlockDemo
```

### 清理构建文件

要清理构建生成的所有文件，请运行：

```bash
make clean
```

## 示例说明

### 死锁演示

死锁演示程序展示了两个任务（Task1和Task2）之间因为互斥锁获取顺序不当导致的死锁现象：

- 任务1先获取互斥锁1，然后尝试获取互斥锁2
- 任务2先获取互斥锁2，然后尝试获取互斥锁1

当两个任务同时持有一个互斥锁并等待对方释放另一个互斥锁时，就会导致死锁。

### 死锁检测

项目实现了一个死锁检测机制，可以：

- 实时监控系统中互斥锁的使用情况
- 构建资源分配图用于死锁检测
- 在检测到死锁时提供详细的任务和互斥锁状态信息
- 可配置检测周期和行为

## 退出程序

在终端中按 `Ctrl+C` 可以正常退出程序。
