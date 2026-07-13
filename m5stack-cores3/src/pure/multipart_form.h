// multipart/form-data のプリアンブル・エピローグ生成(STT への WAV アップロード用)。
// デバイス非依存の純関数(native 環境でテストする)。生成した preamble の直後に
// ファイル本体(生バイト列)を続け、その後 epilogue を送ると 1 つの multipart body になる
#pragma once
#include <string>
#include <utility>
#include <vector>

namespace multipart_form {

struct Parts {
    std::string preamble;
    std::string epilogue;
};

// boundary を使い、テキストフィールド(fields)とファイルパートのヘッダを組み立てる
Parts buildFileUploadParts(const std::string& boundary,
                           const std::vector<std::pair<std::string, std::string>>& fields,
                           const std::string& fileFieldName, const std::string& filename,
                           const std::string& fileContentType);

// Content-Type ヘッダの値("multipart/form-data; boundary=...")
std::string contentTypeHeader(const std::string& boundary);

}  // namespace multipart_form
