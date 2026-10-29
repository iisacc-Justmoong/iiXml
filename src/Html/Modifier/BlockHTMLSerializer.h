#pragma once

#include "src/Html/Modifier/CombineBlock.h"
#include "src/Html/Modifier/DivideBlock.h"

#include <QObject>
#include <QString>

#include <string>
#include <vector>

namespace iiHtmlBlock {

class BlockHTMLSerializer : public QObject {
    Q_OBJECT

public:
    explicit BlockHTMLSerializer(QObject* parent = nullptr);

    [[nodiscard]] bool SerializeBlocks(const std::vector<DivideBlock::ElementInfo>& blocks);
    [[nodiscard]] bool SerializeCombinedBlocks(const std::vector<CombineBlock::CombinedBlock>& combined_blocks);
    [[nodiscard]] bool SerializeMergedBlocks(
        const std::vector<DivideBlock::ElementInfo>& blocks,
        const std::vector<CombineBlock::CombinedBlock>& combined_blocks
    );

    void Clear();

    [[nodiscard]] const std::string& GetHTMLText() const;
    [[nodiscard]] const std::string& GetError() const;

signals:
    void Serialized(const QString& html);
    void SerializeFailed(const QString& reason);

private:
    void SetHTML(std::string html);
    void SetError(const std::string& error);

    std::string html_;
    std::string error_;
};

} // namespace iiHtmlBlock
