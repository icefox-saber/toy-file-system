# 简易inode文件系统

.  
├── FS  
│   ├── BDS.c#磁盘服务器  
│   ├── FC.c#文件系统客户端  
│   ├── FS.c#文件系统服务器，磁盘客户端  
│   └── Makefile  
├── Makefile  
├── README.md  
├── include  
│   ├── tcp_buffer.h#tcp发送接受函数  
│   ├── tcp_utils.h#tcp服务端客户端封装  
│   └── thpool.h#线程池封装  
├── lib  
│   ├── tcp_buffer.c  
│   ├── tcp_utils.c  
│   └── thpool.c  
├── report.pdf  
└── test.pdf  

3 directories, 14 files  

FS中，由超级块层使用磁盘服务器来管理块的分配

inode文件系统使用超级块的服务，管理inode文件

命令列表使用inode文件系统的服务，对客户端使用TCP连接，提供命令列表如ls mk mkdir等等命令