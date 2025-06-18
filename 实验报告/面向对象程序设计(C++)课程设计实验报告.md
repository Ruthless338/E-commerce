
## 1 前言  
### 1.1 题目简要介绍
本题目旨在设计并实现一个简单的电商交易平台，提供用户管理、商品管理、交易管理等功能。  
本作业包含三个题目，使得该电商交易平台功能逐步增强。前两个题目为单机版，运行时体现为一个进程。第三个题目为网络版，要求采用CS结构，客户端和服务器端为不同的进程。  
其中题目一账户管理子系统和商品管理子系统主要完成用户管理和商品管理，题目二交易管理子系统主要完成交易管理，题目三网络版在功能上要求与单机版完全相同，使用socket进行网络通信，分离客户端与服务端。

### 1.2 需求理解

根据项目目标，我对需求进行了如下理解：

用户管理：系统应支持用户注册新账户、登录、修改密码、账户充值以及查询余额。用户分为消费者和商家，拥有不同权限。

商品管理：商家可以添加、修改自己的商品信息（名称、描述、价格、库存、图片等）。平台支持对某一商品品类（如图书、服装、食品）进行统一打折。

商品浏览与搜索：所有用户可以浏览平台上的所有商品。系统应提供搜索功能，支持按关键字、类别、价格区间进行筛选。

购物车：消费者可以将感兴趣的商品加入购物车，并能在购物车中修改商品数量或移除商品。

订单系统：消费者可以从购物车中选择商品生成订单。订单在创建后有支付时限（例如5分钟），超时未支付将自动取消。支付成功后，将扣除消费者余额，增加商家余额，并相应扣减商品库存。

技术架构：采用C/S架构，服务端使用C++/Qt开发，负责处理所有业务逻辑和数据持久化；客户端使用QML进行UI开发，通过网络与服务端进行交互。

数据持久化：所有系统数据（用户信息、商品信息、购物车、订单）需要被持久化存储，本项目使用JSON文件格式进行存储。

并发处理：服务端需要能够同时处理多个客户端的连接和请求，保证数据操作的线程安全。

## 2 总体设计

### 2.1 单机版
在单机版的系统中，采取了前后端分离的架构，前端由QML开发，后端由C++开发，无数据库，使用JSON文件进行数据持久化。  
#### 前端
QML开发，通过QT Quick提供的组件和组件属性，实现界面的搭建。接收后端数据更新信号并及时刷新，通过main.cpp中注册的类和信号实现与后端进行通信。
1. 主界面：展示登录/注册、商户信息、修改余额等按钮和全部的商品信息以及搜索栏。  
   
2. 登录/注册界面
3. 消费者修改余额界面
4. 消费者查看购物车界面
5. 消费者订单界面
6. 商家添加商品信息界面
7. 商家编辑商品信息界面
8. 消费者/商家修改密码界面
9.  平台设置折扣界面

#### 后端
1. 主函数：注册C++类到QML并启动QML界面
   
2. 用户管理：包括用户基类、消费者/商家子类、用户管理类
3. 商品管理：包括商品基类、图书/服装/食品子类、商品管理类、商品模型类
4. 购物车管理：购物车类、购物车管理类
5. 订单管理：订单类、订单管理类
6. 文件管理：文件管理类

### 2.2 网络版

本系统采用经典的客户端-服务器（Client-Server）分层架构。该架构将用户界面（UI）与业务逻辑和数据存储完全分离，提高了系统的可维护性、可扩展性和安全性。

#### 服务端子系统 (Server Subsystem)
作为系统的后端核心，服务端是一个多线程的TCP服务器。它不包含任何图形界面，主要职责是：

1. 监听并接受客户端的网络连接。

2. 为每个客户端连接创建一个独立的线程进行处理，实现高并发。

3. 解析客户端发来的JSON格式请求。

4. 执行核心业务逻辑，主要包括用户管理、商品管理、购物车管理、订单管理等单机版中的全部逻辑功能。

5. 通过FileManager模块与数据文件交互，实现数据的持久化存取。

6. 将处理结果封装成JSON格式响应并发送回客户端。

7. 保证多线程环境下对共享数据（文件）访问的原子性和一致性。

#### 客户端子系统 (Client Subsystem)
作为系统的用户交互前端，客户端前端界面在单机版的基础上几乎没有改动，C++逻辑层的修改。主要职责是：

1. 提供用户友好的图形界面。

2. 捕获用户操作（如点击按钮、输入文本）。

3. 将用户操作转换为特定格式的JSON请求。

4. 通过NetworkClient模块与服务端建立TCP连接并发送请求。

5. 接收并解析服务端的JSON响应。

6. 根据响应结果更新UI界面（如显示商品列表、更新用户余额、提示操作成功或失败）。

7. 通过GlobalState对象管理全局用户状态（如登录状态、用户名、余额）。

子系统间关系：
客户端和服务端通过TCP/IP网络协议进行通信。通信的数据载体是自定义的、基于JSON的文本协议。客户端是请求的发起方，服务端是请求的处理和响应方。数据存储（JSON文件）仅由服务端直接访问，客户端通过服务端的接口间接操作数据，实现了数据的封装和保护。

```mermaid

graph TD
    subgraph 客户端 (Client Subsystem)
        UI[QML用户界面]
        ClientManagers[C++业务代理层<br>(AuthManager, ProductModel, etc.)]
        NetworkClient[网络客户端<br>(NetworkClient)]
    end

    subgraph 服务端 (Server Subsystem)
        Server[TCP服务器 (Server)]
        ClientHandler[客户端处理器<br>(ClientHandler)]
        ServerManagers[C++业务逻辑层<br>(ServerAuthManager, etc.)]
        FileManager[文件管理器 (FileManager)]
    end

    subgraph 数据存储 (Data Persistence)
        DataStore[JSON文件<br>(users.json, products.json, etc.)]
    end

    UI -- 用户操作 --> ClientManagers
    ClientManagers -- 封装请求 --> NetworkClient
    NetworkClient -- JSON/TCP 请求 --> Server
    Server -- 创建 --> ClientHandler
    ClientHandler -- 解析请求并调用 --> ServerManagers
    ServerManagers -- 操作数据 --> FileManager
    FileManager -- 读/写 (带锁) --> DataStore
    ServerManagers -- 返回结果 --> ClientHandler
    ClientHandler -- 封装响应 --> Server
    Server -- JSON/TCP 响应 --> NetworkClient
    NetworkClient -- 解析响应 --> ClientManagers
    ClientManagers -- 更新数据/状态 --> UI
```

## 3 详细设计
### 3.1 服务端子系统详细设计

服务端的核心是围绕业务逻辑管理器展开的，由Server类负责网络监听，ClientHandler类负责并发处理。

主要类及职责:

Server: 继承自QTcpServer，负责监听指定端口，在接收到新连接时，为该连接创建一个ClientHandler实例，并将其移动到一个新的QThread中运行。

ClientHandler: 负责处理单个客户端的所有通信。它包含一个QTcpSocket，通过信号槽机制异步读取和处理数据。它解析收到的JSON请求，根据action字段调用相应的管理器方法，并将结果封装后回传。

ServerAuthManager: 处理所有与用户身份验证和账户相关的业务，如登录、注册、修改密码、充值、扣款等。

ServerProductManager: 处理所有与商品相关的业务，如增、删（本系统未实现）、改、查，以及设置品类折扣。

ServerShoppingCartManager: 管理所有用户的购物车数据。

ServerOrderManager: 负责订单的整个生命周期，包括创建订单（冻结库存）、支付订单（扣款、转移资金、确认库存）、查询订单历史、处理超时订单（释放库存）等。

FileManager: 静态工具类，负责所有数据文件的读写操作。内部使用QMutex来保证文件访问的线程安全，防止多个ClientHandler线程同时写文件导致数据损坏。

User, Product, Order及其派生类: 数据模型，定义了系统中核心实体的数据结构和基本行为。

类关系图:

Generated mermaid
classDiagram
    class Server {
        +startServer(port)
        #incomingConnection(socketDescriptor)
    }
    class ClientHandler {
        -m_socket: QTcpSocket*
        -m_authManager_s: ServerAuthManager*
        -m_productManager_s: ServerProductManager*
        -m_cartManager_s: ServerShoppingCartManager*
        -m_orderManager_s: ServerOrderManager*
        +process()
        -processMessage(request)
        -handleLogin(payload)
        -handleGetProducts(payload)
    }
    class ServerAuthManager {
        +verifyLogin(user, pass)
        +registerUser(...)
        +deductBalance(user, amount)
    }
    class ServerProductManager {
        +getAllProducts()
        +addProduct(...)
        +freezeStock(product, qty)
        +confirmStockDeduction(product, qty)
    }
    class ServerShoppingCartManager {
        +addItem(user, product, qty)
        +getCartItems(user)
    }
    class ServerOrderManager {
        +prepareOrder(...)
        +payOrder(...)
    }
    class FileManager {
        <<static>>
        -fileMutex: QMutex
        +loadAllUsers()
        +saveProducts(products)
    }
    class Product {
        <<abstract>>
        #name
        #basePrice
        +getPrice()
    }
    class Book {
        +discount: static double
    }
    class Clothing {
        +discount: static double
    }
    class Food {
        +discount: static double
    }

    QTcpServer <|-- Server
    Server "1" o-- "*" ClientHandler : creates
    QThread o-- ClientHandler : runs in

    ClientHandler o-- ServerAuthManager
    ClientHandler o-- ServerProductManager
    ClientHandler o-- ServerShoppingCartManager
    ClientHandler o-- ServerOrderManager

    ServerAuthManager ..> FileManager : uses
    ServerProductManager ..> FileManager : uses
    ServerShoppingCartManager ..> FileManager : uses
    ServerOrderManager ..> FileManager : uses
    
    ServerProductManager o-- "*" Product
    
    Product <|-- Book
    Product <|-- Clothing
    Product <|-- Food
IGNORE_WHEN_COPYING_START
content_copy
download
Use code with caution.
Mermaid
IGNORE_WHEN_COPYING_END
### 3.2 客户端子系统详细设计

客户端通过C++后端代理与QML前端界面结合，实现了模型-视图（Model-View）和关注点分离。

主要类及职责:

main.cpp: 应用程序入口，初始化QML引擎，创建并注册所有C++单例和模型到QML上下文中。

NetworkClient: 单例模式。封装QTcpSocket，负责与服务端建立和维护唯一的TCP连接，提供发送请求和接收响应的接口。通过信号responseReceived将收到的数据分发出去。

GlobalState: 单例模式，继承自QObject。通过Q_PROPERTY暴露当前用户的登录状态（用户名、用户类型、余额）给QML。C++业务逻辑在操作成功后（如登录、充值）会更新此对象，QML界面通过属性绑定自动响应变化。

AuthManager: 静态工具类，封装了所有用户认证相关的网络请求。其核心是sendRequestAndWait方法，该方法使用QEventLoop和QTimer将异步的网络请求“伪同步化”，使QML调用C++方法能像调用普通函数一样等待并获取返回结果。它为每个请求附加一个唯一的requestId，以确保收到的响应与发出的请求正确匹配。

ProductModel: 继承自QAbstractListModel，为QML中的ListView提供商品数据。它通过AuthManager::sendRequestAndWait向服务器请求商品列表，并将返回的JSON数据存储在内部的QList<QVariantMap>中。增、改、查等操作都转化为网络请求，成功后刷新整个模型。

ShoppingCart: 封装购物车相关的所有操作。添加、删除、修改数量等操作都会调用AuthManager发送网络请求，操作成功后会重新从服务器拉取最新的购物车数据来更新自身状态。

OrderManager: 封装订单相关的所有操作，如从购物车准备订单、支付订单、获取历史订单等，所有操作均通过网络请求完成。

类关系图:

Generated mermaid
classDiagram
    direction LR
    class QML_UI {
        <<QML>>
        LoginView
        ProductView
        CartView
    }
    class GlobalState {
        <<Singleton>>
        +username
        +userType
        +balance
        +logout()
    }
    class AuthManager {
        <<static>>
        +verifyLogin(...)
        +registerUser(...)
        +sendRequestAndWait(...)
    }
    class ProductModel {
        <<Model>>
        -m_productsData: QList~QVariantMap~
        +search(...)
        +addProduct(...)
    }
    class ShoppingCart {
        -m_cartItems: QList~QVariantMap~
        +addItem(...)
        +getItems()
    }
    class OrderManager {
        +prepareOrderFromCart()
        +payOrder(orderId)
    }
    class NetworkClient {
        <<Singleton>>
        -m_socket: QTcpSocket*
        +connectToServer()
        +sendRequest(json)
        +responseReceived(json)
    }

    QML_UI ..> GlobalState : Binds to properties
    QML_UI ..> AuthManager : Calls methods
    QML_UI ..> ProductModel : Uses as model
    QML_UI ..> ShoppingCart : Calls methods
    QML_UI ..> OrderManager : Calls methods

    AuthManager ..> NetworkClient : Uses to send requests
    ProductModel ..> AuthManager : Uses for requests
    ShoppingCart ..> AuthManager : Uses for requests
    OrderManager ..> AuthManager : Uses for requests

    AuthManager ..> GlobalState : Updates on login/recharge
    OrderManager ..> GlobalState : Updates balance on payment
IGNORE_WHEN_COPYING_START
content_copy
download
Use code with caution.
Mermaid
IGNORE_WHEN_COPYING_END
### 3.3 数据存储说明

本系统不使用关系型数据库，所有数据均通过FileManager以JSON格式持久化到本地文件中。文件路径在代码中为硬编码（例如 "D:/.../data/users.json"）。

users.json: 存储所有用户信息的JSON数组。

结构: [ {user_object_1}, {user_object_2}, ... ]

user_object 字段说明:
| 字段名 | 类型 | 说明 |
| :--- | :--- | :--- |
| name | string | 用户名 (唯一) |
| password | string | 经过SHA-256哈希后的密码摘要 |
| balance | number | 用户账户余额 |
| type | string | 用户类型 ("Consumer" 或 "Merchant") |

products.json: 存储所有商品信息及品类折扣。

结构: { "categories": {...}, "products": [...] }

categories 字段说明:
| 字段名 | 类型 | 说明 |
| :--- | :--- | :--- |
| 图书 | number | 图书品类的折扣率 (0.0-1.0) |
| 服装 | number | 服装品类的折扣率 (0.0-1.0) |
| 食品 | number | 食品品类的折扣率 (0.0-1.0) |

products 数组中 product_object 字段说明:
| 字段名 | 类型 | 说明 |
| :--- | :--- | :--- |
| name | string | 商品名称 |
| description | string | 商品描述 |
| price | number | 商品基础价格（未打折） |
| stock | number | 商品总库存 |
| frozenStock | number | 被订单冻结的库存数量 |
| category | string | 商品类别 ("图书", "服装", "食品") |
| imagePath | string | 商品图片的本地路径 |
| merchantUsername| string | 所属商家的用户名 |

shoppingCart.json: 存储所有用户的购物车。

结构: { "username1": {cart_object_1}, "username2": {cart_object_2}, ... }

cart_object 字段说明:

key: "商品名_商家用户名" 构成的唯一标识符。

value (number): 该商品在购物车中的数量。

order.json: 存储所有订单信息。

结构: [ {order_object_1}, {order_object_2}, ... ]

order_object 字段说明:
| 字段名 | 类型 | 说明 |
| :--- | :--- | :--- |
| orderId | string | 订单的唯一ID (UUID) |
| consumerUsername | string | 下单的消费者用户名 |
| creationTime | string | 订单创建时间 (ISO 8601格式) |
| status | string | 订单状态 ("Pending", "Paid", "Cancelled") |
| items | array | 订单中的商品项数组 |

items 数组中 item_object 字段说明:
| 字段名 | 类型 | 说明 |
| :--- | :--- | :--- |
| productName | string | 商品名称 |
| merchantUsername| string | 商品所属商家 |
| quantity| number | 购买数量 |

### 3.4 接口协议说明

底层承载协议: TCP/IP。

消息帧: 客户端与服务端之间的每个JSON消息都以换行符 \n 作为结束标记，用于解决粘包问题。

数据格式: 所有交换的数据均为UTF-8编码的JSON对象。

请求 (Client -> Server):

Generated json
{
    "action": "action_name",
    "requestId": "unique_request_id",
    "payload": { ... }
}
IGNORE_WHEN_COPYING_START
content_copy
download
Use code with caution.
Json
IGNORE_WHEN_COPYING_END

action: (string, 必选) 表示要执行的操作，如"login", "getProducts", "addToCart"。

requestId: (string, 必选) 客户端生成的唯一请求ID（UUID），用于匹配响应。

payload: (object, 可选) 包含该操作所需的具体参数。

响应 (Server -> Client):

Generated json
{
    "response_to_action": "action_name",
    "requestId": "unique_request_id",
    "status": "success" | "error",
    "data": { ... },
    "message": "error_description"
}
IGNORE_WHEN_COPYING_START
content_copy
download
Use code with caution.
Json
IGNORE_WHEN_COPYING_END

response_to_action: (string, 必选) 回显请求中的action。

requestId: (string, 必选) 回显请求中的requestId。

status: (string, 必选) 操作结果，"success"表示成功，"error"表示失败。

data: (object, 仅在status为success时出现) 操作成功时返回的数据。

message: (string, 仅在status为error时出现) 操作失败时的错误信息。

示例接口:

用户登录:

action: "login"

payload: {"username": "testuser", "password": "123456"}

data (on success): {"username": "testuser", "type": "Consumer", "balance": 100.0}

获取商品列表:

action: "getProducts"

payload: (空)

data (on success): {"products": [ {...}, {...} ]}

准备订单:

action: "prepareOrder"

payload: {"itemsData": [{"productName":"C++ Primer", "merchantUsername":"bookstore", "quantity":1}]}

data (on success): {"orderId": "...", "total": 99.0, "status": "Pending", ...}

## 4 实现
### 4.1 实现过程中遇到的主要问题和解决方案

问题：C++/QML前端开发与对接

描述: 缺乏复杂的C++ GUI开发经验，特别是QML与C++后端的有效集成。如何将QML的用户操作传递给C++，以及如何将C++的数据和状态变化反映到QML界面，是初期面临的主要挑战。

解决方案:

注册C++类型到QML: 采用qmlRegisterSingletonType将AuthManager、ShoppingCart、OrderManager等业务代理类注册为QML中的单例。这使得在QML中可以直接通过类名调用其Q_INVOKABLE方法，如 AuthManager.verifyLogin(...)。

暴露数据模型: 对于列表数据（如商品列表），将ProductModel（继承自QAbstractListModel）实例通过QQmlApplicationEngine::rootContext()->setContextProperty()暴露为全局属性。QML中的ListView可以直接将model属性绑定到这个C++对象上。

全局状态管理: 创建GlobalState单例，使用Q_PROPERTY宏定义用户状态（如username, balance），并提供NOTIFY信号。QML界面元素（如Text）的属性可以直接绑定到这些C++属性上。当C++端（如登录成功后）修改这些属性时，会自动触发NOTIFY信号，QML界面随之自动更新，实现了响应式UI。

问题：头文件循环依赖

描述: 在设计中，User类需要调用FileManager进行用户注册时的存在性检查，而FileManager在加载用户时又需要创建User对象，导致user.h和filemanager.h相互#include，引发编译错误。

解决方案: 采用**前向声明（Forward Declaration）**来打破编译时依赖。

在user.h中，移除#include "filemanager.h"。

在User类定义前，添加class FileManager;的前向声明。

将需要使用FileManager完整定义的成员函数（如User::registerUser）的实现从user.h移到user.cpp中。

在user.cpp中#include "filemanager.h"，此时编译器拥有FileManager的完整定义，可以正常编译。

问题：服务端并发访问与数据一致性

描述: 服务端为每个客户端创建一个线程，这些线程会并发地调用FileManager读写共享的JSON文件。若不加控制，可能导致“写-写”或“读-写”冲突，造成数据文件损坏或读取到不一致的中间状态。

解决方案:

线程模型: 采用“一个连接一个线程”（Thread-per-Connection）模型。在Server::incomingConnection中，将新创建的ClientHandler对象通过moveToThread()方法迁移至一个新的QThread实例中执行，隔离了不同客户端的处理逻辑。

互斥锁: 在FileManager中定义一个静态的QMutex成员fileMutex。在FileManager所有执行文件I/O操作的静态方法（如saveUser, loadProducts）的入口处，使用QMutexLocker对fileMutex进行加锁。QMutexLocker利用了RAII（资源获取即初始化）思想，在对象创建时自动获取锁，在对象离开作用域时（函数返回或抛出异常）自动释放锁，确保了对数据文件的访问是原子性的，从而避免了并发冲突。

### 4.2 想法、经验与教训

想法与改进:

1. 异步客户端  
   在实现网络版的过程中，由难到易我一共想到了两种实现方式：异步客户端、伪同步客户端。  
   由于单机版我做了UI界面，所以如果要实现第一种异步客户端，需要将QML中许多形如QObject.funtion的静态或非静态函数全部重写为Connections信号function监听方式，单机版main.cpp中大量通过注册的类函数逻辑已经转移到了服务端，所以这种修改是必不可少的，但由于我单机版的QML与C++逻辑耦合度太高，所以异步客户端改造和debug工作量太大，时间有限我决定退而求其次尽可能不修改QML代码。  
   对于第二种伪同步客户端方式也就是我目前采用的方式，客户端的AuthManager::sendRequestAndWait方法使用了QEventLoop，这会阻塞调用线程直到服务端返回结果，由于服务端和客户端都是运行在本地，端口之间通信会更快，所以这里阻塞线程的时间会大幅度降低，但如果操作耗时较长可能导致UI卡顿。  
   如果要真正的应用，选择第一种异步客户端会优化多用户的体验和效率，但限于时间和精力，我选择了第一种较为简单的方式，规避了对QML的大幅度改动。

2. 数据库替代文件  
   JSON文件作为数据存储方案，在并发量增大和数据量增多时性能会急剧下降，且不利于复杂查询。未来可将FileManager替换为一个数据库管理模块，使用轻量级的SQLite（用于单文件部署）或更强大的PostgreSQL/MySQL，能显著提升性能和数据管理的健壮性。

3. 健壮的网络协议  
   当前的换行符分包方案在某些极端情况下可能失效。更健壮的方案是使用“长度前缀”协议，即在每条JSON消息前附加一个固定长度的字段，指明后续JSON数据的字节数。

经验与教训:

分层设计的重要性: 本项目清晰的C/S分层、服务端内部的逻辑分层（网络层、业务层、数据访问层）以及客户端的MVVM思想（QML-View, C++-ViewModel, Server-Model），极大地降低了代码的耦合度，使得各模块可以独立开发和测试，提高了开发效率和代码质量。

并发编程需谨慎: 多线程编程的核心是正确识别和保护共享资源。QMutex是保证线程安全的基本工具，而QMutexLocker是比手动lock()/unlock()更安全、更推荐的使用方式。

接口先行: 在C/S开发中，预先定义好清晰、稳定的接口协议（API）至关重要。协议一旦确定，客户端和服务端就可以并行开发，减少了后续联调的障碍。在本项目中，JSON协议的设计就是这一思想的体现。