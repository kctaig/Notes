#include <cstring>
#include <memory>

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
        return *this;  // 4. 返回*this
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