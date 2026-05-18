# 软件说明

## 软件依赖

CXX 17+
QT 6.10+
CMake 3.16+
MSVC 2022

## 注意事项

1. 前后端分离，gui模块只负责图形显示，core模块负责业务实现，app模块作为程序入口负责组装其他模块。core对外暴露Iservice接口，gui通过controller进行service的业务编排。
2. 数据库分为两个，project下为项目数据库，存放在用户选择的任意位置。system下为系统数据库（软件级数据，如激活信息，用户信息等项目无关的数据），与exe文件存放在同级目录。所有的dao层访问需继承basedao，basedao内部实现了databasemanager的调用。
3. 软件有新建与切换项目的功能，切换项目时需要清空页面数据，切换数据源后重新加载新的页面数据。打开/切换项目通过DatabaseManager切换sqlite路径及递归调用IDataOperateInterface实现的方式实现
4. 所有页面需要实现IDataOperateInterface，项目切换时会递归调用该接口方法重新加载页面数据。

## 目录说明

### 根目录

```txt
├─cmake          CMake 构建配置目录
├─platform       跨平台适配代码/资源
├─resources      项目静态资源（字体、图标、样式等）
├─src            核心源代码（项目主体）
├─test           单元测试/集成测试代码
├─third_party    需要引入第三方依赖库
└─translations   国际化多语言翻译文件
```

### src目录

#### core目录

```txt
├─config        # 全局配置管理：系统核心配置
├─context       # 应用上下文，单例生命周期管理
├─dao           # 数据访问实现层：对接数据库/存储的具体操作（实现接口层定义）
│  ├─interfaces # DAO内部接口定义
│  ├─project    # 项目模块DAO：项目相关数据增删改查、数据库操作实现
│  └─system     # 系统模块DAO：系统配置、权限、用户等系统数据操作实现
├─depends       # 核心层外部依赖管理
├─interfaces    # 核心抽象接口层：**所有核心模块的接口定义，解耦实现**
│  ├─adapter    # 适配器接口，封装service为rpc接口
│  ├─context    # 上下文接口
│  ├─dao        # 数据访问接口：规范存储操作的抽象（无具体实现）
│  │  ├─project # 项目模块DAO接口
│  │  └─system  # 系统模块DAO接口
│  ├─models     # 数据模型定义：系统统一的数据结构规范

│  │  ├─constants # 全局常量：固定配置、标识、数值常量
│  │  ├─enums    # 枚举类型：状态、类型、业务标识等枚举定义
│  │  └─po       # 持久化对象：数据库映射实体
│  ├─service    # 业务服务接口：规范业务逻辑的抽象
│  └─utils      # 工具类接口：通用工具的抽象约定
├─models        # 数据模型实现层：PO、DTO、VO等数据实体的具体代码实现
├─service       # 业务逻辑实现层：核心业务逻辑、业务规则的具体实现
└─utils         # 核心通用工具：公共方法、加密、格式转换、通用能力封装
```

#### gui目录

```txt
├─adapter       # service适配器：封装rpcService为同步service
├─context       # UI上下文：单例生命周期管理
├─controller    # UI控制器：处理界面交互事件、调用core服务、数据流转
├─depends       # UI层外部依赖管理
├─guiservice    # UI专属服务
├─interfaces    # UI抽象接口层
│  ├─context    # UI上下文接口
│  ├─controller # UI控制器接口
│  ├─guiservice # UI服务接口
│  └─widgets    # UI组件接口
├─mainwindow    # 主窗口管理
│  ├─dialogs    # 弹窗对话框
│  └─menus      # 菜单管理
├─utils         # UI通用工具
├─widgets       # 通用UI组件：自定义控件、公共按钮/表单/表格等基础组件
└─workspace     # 业务工作区：主界面核心功能模块（核心UI业务入口）
    ├─device    # 设备管理工作区
    ├─processing# 数据处理工作区
    ├─realtime  # 实时数据工作区
    └─well      # 井管理工作区
```
