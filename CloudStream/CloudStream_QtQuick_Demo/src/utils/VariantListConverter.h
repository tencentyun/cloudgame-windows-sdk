#ifndef VARIANTLISTCONVERTER_H
#define VARIANTLISTCONVERTER_H

#include <QVariantList>
#include <QByteArray>
#include <vector>

/**
 * @brief QVariant列表转换工具类
 * 
 * 用于将QVariantList或QStringList转换为C风格的字符串数组指针，
 * 同时保证内存安全和生命周期管理。
 */
class VariantListConverter
{
public:
    /**
     * @brief 转换结果结构体
     * 
     * 包含两个成员：
     * - byteArrays: 实际持有字符串数据的容器
     * - pointers: 指向byteArrays中数据的C风格指针数组
     * 
     * 注意：pointers的生命周期依赖于byteArrays，使用时需确保byteArrays有效
     */
    struct ConvertResult {
        std::vector<QByteArray> byteArrays;  // 持有实际内存数据
        std::vector<const char*> pointers;   // 指向byteArrays数据的C风格指针
        
        // 默认构造函数
        ConvertResult() = default;
        
        // 禁用拷贝构造和拷贝赋值（避免指针失效）
        ConvertResult(const ConvertResult&) = delete;
        ConvertResult& operator=(const ConvertResult&) = delete;
        
        // 允许移动构造和移动赋值（保证性能）
        ConvertResult(ConvertResult&&) = default;
        ConvertResult& operator=(ConvertResult&&) = default;
    };

    /**
     * @brief 将QVariantList转换为C风格字符串数组
     * 
     * @param list 输入的QVariantList，每个元素将被转换为QString再转为UTF-8字符串
     * @return ConvertResult 包含转换后的数据和指针
     */
    static ConvertResult convert(const QVariantList& list);
    
    /**
     * @brief 将QStringList转换为C风格字符串数组
     * 
     * @param list 输入的QStringList
     * @return ConvertResult 包含转换后的数据和指针
     */
    static ConvertResult convert(const QStringList& list);
};

#endif // VARIANTLISTCONVERTER_H