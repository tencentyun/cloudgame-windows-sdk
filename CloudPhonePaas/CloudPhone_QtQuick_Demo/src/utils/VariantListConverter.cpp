#include "VariantListConverter.h"

VariantListConverter::ConvertResult VariantListConverter::convert(const QVariantList& list)
{
    ConvertResult result;
    result.byteArrays.reserve(list.size());
    result.pointers.reserve(list.size());

    for (const QVariant& v : list) {
        QByteArray arr = v.toString().toUtf8();
        if (arr.isEmpty() || arr.constData()[arr.size() - 1] != '\0') {
            arr.append('\0'); // 保证以\0结尾
        }
        result.byteArrays.emplace_back(std::move(arr));
        result.pointers.push_back(result.byteArrays.back().constData());
    }
    return std::move(result); // 使用移动语义返回
}

VariantListConverter::ConvertResult VariantListConverter::convert(const QStringList& list)
{
    QVariantList vlist;
    for (const QString& s : list) vlist << s;
    return convert(vlist); // 内部调用已使用移动语义
}
