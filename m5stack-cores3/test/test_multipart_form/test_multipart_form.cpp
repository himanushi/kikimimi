// multipart/form-data 組み立て(issue 00014、STT への WAV アップロード用)のテスト。
// 仕様: preamble はテキストフィールド → ファイルパートヘッダの順で、各境界は
// "--boundary\r\n" で始まる。epilogue は最終境界 "--boundary--\r\n"。
#include <unity.h>

#include "pure/multipart_form.h"

using multipart_form::buildFileUploadParts;
using multipart_form::contentTypeHeader;
using multipart_form::Parts;

void setUp() {}
void tearDown() {}

static bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

static void test_content_type_header_includes_boundary() {
    TEST_ASSERT_EQUAL_STRING("multipart/form-data; boundary=XYZ", contentTypeHeader("XYZ").c_str());
}

static void test_preamble_contains_text_fields_in_order() {
    Parts parts = buildFileUploadParts(
        "B1", {{"model", "gpt-4o-mini-transcribe"}, {"language", "ja"}}, "file", "audio.wav",
        "audio/wav");
    size_t modelPos = parts.preamble.find("gpt-4o-mini-transcribe");
    size_t langPos = parts.preamble.find("\"language\"");
    TEST_ASSERT_TRUE(modelPos != std::string::npos);
    TEST_ASSERT_TRUE(langPos != std::string::npos);
    TEST_ASSERT_TRUE(modelPos < langPos);
    TEST_ASSERT_TRUE(contains(parts.preamble, "Content-Disposition: form-data; name=\"model\""));
    TEST_ASSERT_TRUE(contains(parts.preamble, "ja"));
}

static void test_preamble_file_part_header_has_filename_and_content_type() {
    Parts parts = buildFileUploadParts("B1", {}, "file", "audio.wav", "audio/wav");
    TEST_ASSERT_TRUE(
        contains(parts.preamble, "Content-Disposition: form-data; name=\"file\"; filename=\"audio.wav\""));
    TEST_ASSERT_TRUE(contains(parts.preamble, "Content-Type: audio/wav"));
}

static void test_preamble_ends_with_blank_line_before_file_body() {
    Parts parts = buildFileUploadParts("B1", {}, "file", "audio.wav", "audio/wav");
    // ファイル本体の直前は必ず空行(\r\n\r\n)で終わる(multipart 仕様)
    TEST_ASSERT_TRUE(parts.preamble.size() >= 4);
    std::string tail = parts.preamble.substr(parts.preamble.size() - 4);
    TEST_ASSERT_EQUAL_STRING("\r\n\r\n", tail.c_str());
}

static void test_epilogue_is_final_boundary() {
    Parts parts = buildFileUploadParts("B1", {}, "file", "audio.wav", "audio/wav");
    TEST_ASSERT_EQUAL_STRING("\r\n--B1--\r\n", parts.epilogue.c_str());
}

static void test_all_boundaries_use_given_boundary_string() {
    Parts parts = buildFileUploadParts("MyBoundary123", {{"model", "m"}}, "file", "a.wav", "audio/wav");
    TEST_ASSERT_TRUE(contains(parts.preamble, "--MyBoundary123\r\n"));
    TEST_ASSERT_TRUE(contains(parts.epilogue, "--MyBoundary123--"));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_content_type_header_includes_boundary);
    RUN_TEST(test_preamble_contains_text_fields_in_order);
    RUN_TEST(test_preamble_file_part_header_has_filename_and_content_type);
    RUN_TEST(test_preamble_ends_with_blank_line_before_file_body);
    RUN_TEST(test_epilogue_is_final_boundary);
    RUN_TEST(test_all_boundaries_use_given_boundary_string);
    return UNITY_END();
}
