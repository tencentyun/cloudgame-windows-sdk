#include "VariantListConverter.h"

/**
 * @brief 将QVariantList转换为C风格字符串数组
 * 
 * 转换过程：
 * 1. 遍历QVariantList中的每个元素
 * 2. 将每个QVariant转换为QString，再转为UTF-8编码的QByteArray
 * 3. 确保每个字符串以'\0'结尾（C字符串要求）
 * 4. 将QByteArray存储到byteArrays容器中
 * 5. 将指向QByteArray数据的指针存储到pointers容器中
 * 
 * @param list 输入的QVariantList
 * @return ConvertResult 包含转换后的数据和指针的结构体
 */
VariantListConverter::ConvertResult VariantListConverter::convert(const QVariantList& list)
{
    ConvertResult result;
    
    // 预分配空间以提高性能
    result.byteArrays.reserve(list.size());
    result.pointers.reserve(list.size());

    // 遍历列表中的每个元素
    for (const QVariant& v : list) {
        // 转换为UTF-8编码的字节数组
        QByteArray arr = v.toString().toUtf8();
        
        // 确保字符串以'\0'结尾（C字符串标准）
        if (arr.isEmpty() || arr.constData()[arr.size() - 1] != '\0') {
            arr.append('\0');
        }
        
        // 使用移动语义添加到容器中（避免拷贝）
        result.byteArrays.emplace_back(std::move(arr));
        
        // 保存指向最后添加的QByteArray数据的指针
        result.pointers.push_back(result.byteArrays.back().constData());
    }
    
    // 使用移动语义返回结果（避免拷贝）
    return std::move(result);
}

/**
 * @brief 将QStringList转换为C风格字符串数组
 * 
 * 内部实现：先将QStringList转换为QVariantList，再调用convert(QVariantList)方法
 * 
 * @param list 输入的QStringList
 * @return ConvertResult 包含转换后的数据和指针的结构体
 */
VariantListConverter::ConvertResult VariantListConverter::convert(const QStringList& list)
{
    // 将QStringList转换为QVariantList
    QVariantList vlist;
    for (const QString& s : list) {
        vlist << s;
    }
    
    // 调用QVariantList版本的convert方法
    return convert(vlist);
}
