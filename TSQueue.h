//
// Created by Administrator on 2024/3/29.
//

#ifndef TSQUEUE_H
#define TSQUEUE_H

#include <QList>
#include <QMutex>

/**
优点：
    对于模板函数,编译器需要在编译时知道函数的完整定义,以便进行实例化。
    如果将模板函数的定义放在 .cpp 文件中,编译器在编译其他使用该模板函数的文件时,就无法访问其定义,从而导致编译错误。
    因此,模板函数通常需要在头文件中提供完整的定义。
    内联函数:

    内联函数是一种提示编译器在调用点直接插入函数体的函数。
    将内联函数的定义放在头文件中,可以让编译器在编译时直接访问函数体,从而实现内联优化。
    内联函数通常用于简短且频繁调用的函数,以提高性能。
    简化代码结构:

    对于一些简单的函数,如 getter 和 setter 函数,直接在头文件中实现可以简化代码结构。
    这样可以避免为每个简单函数创建单独的 .cpp 文件,从而减少文件的数量和复杂性。
    提高编译速度:
缺点：
    1.违反了声明和定义分离的原则,可能导致代码的可读性和可维护性降低。
    2.如果头文件被多个 .cpp 文件包含,那么函数的定义会被多次编译,增加了编译时间。
    3.如果函数的实现发生更改,所有包含该头文件的 .cpp 文件都需要重新编译,增加了编译时间
*/
template <class T>
class TSQueue : public QList<T>{
public:
    TSQueue(){};
    ~TSQueue(){};
    void enqueue(const T &t) {
        QMutexLocker locker(&m_mutex);
        QList<T>::append(t);  // 直接使用父类的方法
    }
    T dequeue() {
        QMutexLocker locker(&m_mutex);
        T t = nullptr;
        if (!QList<T>::isEmpty())
            t = QList<T>::takeFirst();
        return t;
    }
    bool isEmpty() {
        QMutexLocker locker(&m_mutex);
        return QList<T>::isEmpty();
    }
private:
    QMutex m_mutex;
};


#endif //TSQUEUE_H
