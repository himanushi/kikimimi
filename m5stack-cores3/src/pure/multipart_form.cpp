#include "multipart_form.h"

namespace multipart_form {

std::string contentTypeHeader(const std::string& boundary) {
    return "multipart/form-data; boundary=" + boundary;
}

Parts buildFileUploadParts(const std::string& boundary,
                           const std::vector<std::pair<std::string, std::string>>& fields,
                           const std::string& fileFieldName, const std::string& filename,
                           const std::string& fileContentType) {
    Parts parts;
    std::string& p = parts.preamble;

    for (const auto& field : fields) {
        p += "--" + boundary + "\r\n";
        p += "Content-Disposition: form-data; name=\"" + field.first + "\"\r\n\r\n";
        p += field.second + "\r\n";
    }

    p += "--" + boundary + "\r\n";
    p += "Content-Disposition: form-data; name=\"" + fileFieldName + "\"; filename=\"" + filename +
         "\"\r\n";
    p += "Content-Type: " + fileContentType + "\r\n\r\n";

    parts.epilogue = "\r\n--" + boundary + "--\r\n";
    return parts;
}

}  // namespace multipart_form
