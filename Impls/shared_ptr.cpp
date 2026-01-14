#include "iostream"

template <class T>
struct SpControlBlock {
    T* m_data;
    std::atomic<int> m_refcnt;  // 只要T是线程安全的，这里就是线程安全的

    explicit SpControlBlock(T* ptr) : m_data(ptr), m_refcnt(1) {}

    SpControlBlock(const SpControlBlock&&) = delete;

    void incref() noexcept { m_refcnt.fetch_add(1, std::memory_order_relaxed); }

    void decref() noexcept {
        /**
         * 注意:原子变量不要在函数或者表达式中多次出现，会出现竞态条件
         * m_cb->m_refcnt.fetch_sub(1);
         * if (m_cb->m_refcnt.load() == 0) { delete m_cb; }
         */
        if (m_refcnt.fetch_sub(1, std::memory_order_relaxed) == 1) { delete this; }
    }

    long cntref() const noexcept { return m_refcnt.load(std::memory_order_relaxed); }

    ~SpControlBlock() { delete m_data; }
};

template <class T>
struct SharedPtr {
    SpControlBlock<T>* m_cb;

    SharedPtr(std::nullptr_t = nullptr) noexcept : m_cb(nullptr) {}

    // 避免隐式构造
    explicit SharedPtr(T* ptr) : m_cb(new SpControlBlock<T>{ptr}) {}

    SharedPtr(const SharedPtr& other) : m_cb(other.m_cb) { m_cb->incref(); }

    ~SharedPtr() noexcept { m_cb->decref(); }

    long use_count() noexcept { return m_cb->cntref(); }

    bool unique() noexcept { return m_cb->cntref() == 1; }

    T* get() const noexcept { return m_cb->m_data; }
    T* operator->() const noexcept { return m_cb->m_data; }
    T& operator*() const noexcept { return *m_cb->m_data; }
};

template <class T, class... Args>
SharedPtr<T> makeShared(Args&&... args) {
    return SharedPtr<T>(new T(std::forward<Args>(args)...));
}

struct Student {
    const char* name;
    int age;

    explicit Student(const char* name_, int age_) : name(name_), age(age_) {
        std::cout << "Student " << name << " Constructor" << std::endl;
    }

    Student(Student&&) = delete;

    ~Student() { std::cout << "Student " << name << " Destructor" << std::endl; }
};

int main() {
    SharedPtr<Student> p0 = makeShared<Student>("Alice", 20);
    SharedPtr<Student> p1(new Student("Bob", 22));
    SharedPtr<Student> p2 = p0;  // 浅拷贝
    SharedPtr<Student> p3;

    std::cout << p0->name << "," << p0->age << std::endl;
    std::cout << p1->name << "," << p1->age << std::endl;
    std::cout << p2->name << "," << p2->age << std::endl;
    // std::cout << p3->name << "," << p3->age << std::endl;
    std::cout << "p0 use_count: " << p0.use_count() << std::endl;
    std::cout << "p1 use_count: " << p1.use_count() << std::endl;
}