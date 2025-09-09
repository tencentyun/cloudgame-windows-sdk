#ifndef VARIANTLISTCONVERTER_H
#define VARIANTLISTCONVERTER_H

#include <QVariantList>
#include <vector>
#include <QByteArray>

class VariantListConverter
{
public:
    struct ConvertResult {
        std::vector<QByteArray> byteArrays; // 持有内存
        std::vector<const char*> pointers;  // 指向byteArrays数据的指针
        
        // 添加默认构造函数
        ConvertResult() = default;
        
        // 禁用拷贝构造和赋值
        ConvertResult(const ConvertResult&) = delete;
        ConvertResult& operator=(const ConvertResult&) = delete;
        
        // 允许移动语义
        ConvertResult(ConvertResult&&) = default;
        ConvertResult& operator=(ConvertResult&&) = default;
    };

    static ConvertResult convert(const QVariantList& list);
    static ConvertResult convert(const QStringList& list);
};

#endif // VARIANTLISTCONVERTER_H