# C++基础

## 编译链接

## 内存管理

### 内存模型

### RAII

#### C++"五法则"（Rule of Five）

##### 核心理念

如果一个类需要显式定义析构函数、拷贝构造函数或拷贝赋值运算符中的任何一个，那么它很可能需要考虑全部五个特殊成员函数：

1. 析构函数（Destructor）
2. 拷贝构造函数（Copy Constructor）
3. 拷贝赋值运算符（Copy Assignment Operator）
4. 移动构造函数 (Move Constructor)
5. 移动赋值运算符 (Move Assignment Operator)

这种需求通常源于**资源管理责任**。当一个类需要自定义上述任意一个函数时，往往意味着：

- 它正在管理某种资源（动态内存、文件句柄、网络连接、锁等）
- 编译器生成的默认版本无法正确处理该资源

##### 资源管理的完整生命周期

资源管理必须覆盖对象的完整生命周期：

| **阶段**       | **对应的函数** | **关键职责**                                                 |
| :------------- | :------------- | :----------------------------------------------------------- |
| 1. 初始化      | 拷贝构造函数   | 创建新对象时，正确初始化资源（通常需要深拷贝）               |
| 2. 重新赋值    | 拷贝赋值运算符 | 替换现有对象的资源时需要：1. 释放原有资源 2. 拷贝新资源      |
| 3. 清理        | 析构函数       | 正确释放所有拥有的资源，必须是异常安全的                     |
| 4. 高效初始化  | 移动构造函数   | 从临时对象构造新对象：1. 转移资源所有权（非拷贝）2. 置空源对象的资源指针，源对象应处于有效但可析构状态 |
| 5.高效重新赋值 | 移动赋值运算符 | 高效重新赋值： 1. 检查自赋值 2. 释放自身资源 3. 转移新资源所有权 4. 置空源对象资源 |

```
需要自定义析构函数吗？
  ├── 否 → 使用编译器生成版本（零法则优先）
  ↓
 是（手动管理资源）
  ↓
资源应该被共享吗？
  ├── 否（独占所有权）→ 实现：析构 + 移动构造 + 移动赋值
  │                   禁用：拷贝构造 + 拷贝赋值
  ↓
 是（允许共享）
  ↓
实现完整五法则
```

##### 实现模式

```c++
class String {
    char* data;
    size_t length;

  public:
    // ========== 1. 拷贝构造函数 ==========
    String(const String& other)
        : length(other.length), data(new char[length + 1]) {  // 初始化：分配新内存
        std::memcpy(data, other.data, length + 1);
    }

    // ========== 2. 拷贝赋值运算符 ==========
    String& operator=(const String& other) {
        if (this != &other) {  // 1. 检查自赋值
            delete[] data;     // 2. 释放原有资源
            length = other.length;
            data = new char[length + 1];  // 3. 分配并拷贝新资源
            std::memcpy(data, other.data, length + 1);
        }
        return *this;  // 4. 返回*this,支持链式调用
    }

    // ========== 3. 移动构造函数 ==========
    String(String&& other) noexcept  // 标记noexcept!
        : data(other.data),          // 1. 转移所有权（浅拷贝）
          length(other.length) {
        other.data = nullptr;  // 2. 置空源对象
        other.length = 0;      // 3. 设为有效状态
    }

    // ========== 4. 移动赋值运算符 ==========
    String& operator=(String&& other) noexcept {
        if (this != &other) {   // 1. 检查自赋值
            delete[] data;      // 2. 释放自身资源
            data = other.data;  // 3. 转移所有权
            length = other.length;
            other.data = nullptr;  // 4. 置空源对象
            other.length = 0;
        }
        return *this;  // 5. 返回*this
    }

    // ========== 5. 析构函数 ==========
    ~String() {
        delete[] data;  // 清理拥有的资源
    }
};
```

## 多态

### 静态多态

#### 函数重载

#### 模板

### 动态多态

#### 虚函数

#### 纯虚函数和抽象类

## 智能指针

### 智能指针类型

#### auto_ptr

#### unique_ptr

#### shared_ptr

#### weak_ptr

### 共享智能指针实现

引用计数类模板

```c++
template <class T>
struct SpControlBlock {
    T* m_data;
    std::atomic<int> m_refcnt;  // 只要T是线程安全的，这里就是线程安全的

    explicit SpControlBlock(T* ptr) : m_data(ptr), m_refcnt(1) {}

    SpControlBlock(const SpControlBlock&&) = delete;

    void incref() noexcept { m_refcnt.fetch_add(1, std::memory_order_relaxed); }

    void decref() noexcept {
        if (m_refcnt.fetch_sub(1, std::memory_order_relaxed) == 1) { delete this; }
    }

    long cntref() const noexcept { return m_refcnt.load(std::memory_order_relaxed); }

    ~SpControlBlock() { delete m_data; }
};
```

**SharedPtr**模板

```c++
template <class T>
struct SharedPtr {
    SpControlBlock<T>* m_cb;

    SharedPtr(std::nullptr_t = nullptr) noexcept : m_cb(nullptr) {}

    explicit SharedPtr(T* ptr) : m_cb(new SpControlBlock<T>{ptr}) {}

    SharedPtr(const SharedPtr& other) : m_cb(other.m_cb) { m_cb->incref(); }

    ~SharedPtr() noexcept { m_cb->decref(); }

    long use_count() noexcept { return m_cb->cntref(); }

    bool unique() noexcept { return m_cb->cntref() == 1; }

    T* get() const noexcept { return m_cb->m_data; }
    T* operator->() const noexcept { return m_cb->m_data; }
    T& operator*() const noexcept { return *m_cb->m_data; }
};
```

## 类型转换

### static_cast

隐式类型转换，可以实现C++中内置基本数据类型之间的相互转换，enum、struct、int、char、float等，能进⾏类层次间的向上类型转换和向下类型转换（向下不安全，因为没有进⾏动态类型检查）。它不能进⾏⽆关类型(如⾮基类和⼦类)指针之间的转换，也不能作⽤包含底层const的对象；

### dynamic_cast

动态类型转换，⽤于将基类的指针或引⽤安全地转换成派⽣类的指针或引⽤（也可以向上转换），若指针转换失败返回NULL，若引⽤返回失败抛出bad_cast异常。dynamic_cast是在运⾏时进⾏安全性检查；使⽤dynamic_cast⽗类⼀定要有虚函数，否则编译不通过；

### const_cast

把const属性去掉，即将const转换为⾮const（也可以反过来），const_cast只能⽤于指针或引⽤，并且只能改变对象的底层const（顶层const，本⾝是const，底层const，指向对象const）；

### reinterpret_cast

reinterpret是重新解释的意思，此标识符的意思即为将数据的⼆进制形式重新解释，但是不改变其值，有着和C⻛格的强制转换同样的能⼒。它可以转化任何内置的数据类型为其他任何的数据类型，也可以转化任何指针类型为其他的类型。它甚⾄可以转化内置的数据类型为指针，⽆须考虑类型安全或者常量的情形。不到万不得已绝对不⽤（⽐较不安全）

### 动态类型转换的实现原理

- 动态类型转换是C++中⼀种基于运⾏时类型信息（RTTI）的类型转换机制，主要⽤于在类层次结构中进⾏安全的向下转型。动态类型转换使⽤dynamic_cast实现，主要涉及两个步骤，运⾏时类型检查和指针调整

- 运⾏时类型检查：在使⽤dynamic_cast进⾏类型转换时，编译器需要检查给定的指针或引⽤是否确实指向⽬标类型的对象。为了实现这⼀点，编译器利⽤RTTI，在运⾏时获取对象的实际类型。通常，这是通过查询对象的虚函数表（vtable）来实现的，因为vtable包含有关对象类型的信息。如果对象的实际类型与dynamic_cast的⽬标类型兼容（即⽬标类型是源类型的派⽣类），则转换成功。否则转换失败，对于指针类型，dynamic_cast返回空指针，对于引⽤类型抛出std::bad_cast异常

- 指针调整：在多重继承的情况下，⼀个派⽣类对象的内存布局可能包含多个基类⼦对象，这意味着，当将指针从⼀个基类类型转换为另⼀个基类类型时，可能需要调整指针的值。dynamic_cast在运⾏时类型检查成功后，会根据⽬标类型的内存布局进⾏指针调整，以确保指针指向正确的⼦对象。

## 设计模式

### 单例模式

### 工厂模式

### 代理模式

### 观察者模式

## Const

### Const与一级指针结合

一个永远不会忘记的方法，`const` 默认是修饰它左边的符号的，如果左边没有，那么就修饰它右边的符号

1. `const int *p`：左边没有，看右边的一个，是`int`，自然就是`p`指针指向的值不能改变
2. `int const *p`：此时左边有int，其实和上面一样，还是修饰的`int`
3. `int* const p` ：修饰的是`*`，指针不能改变
4. `const int *const p` ：第一个左边没有，所以修饰的是右边的`int`，第二个左边有，所以修饰的是 `*` ，因此指针和指针指向的值都不能改变
5. `const int const * p `：这里两个修饰的都是`int`了，所以重复修饰了，有的编译器可以通过，但是会有警告，你重复修饰了，有的可能直接编译不过去
6. `int const * const p`：同 4

## Lambda表达式

## Function
