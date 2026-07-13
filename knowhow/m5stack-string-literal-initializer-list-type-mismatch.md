# range-based for の `{...}` に String と const char* リテラルを混ぜると型推論に失敗する

- **症状**: `for (const auto& line : {reason, "固定文字列", "  " + info.foo}) { ... }` のようなコードが `unable to deduce 'std::initializer_list<auto>&&'`(`deduced conflicting types for parameter 'auto' ('String' and 'const char*')`)でコンパイルエラーになる
- **原因**: `auto` で受ける range-based for の `{...}` は `std::initializer_list<T>` の `T` を要素から推論する。要素に `String`(`"  " + info.foo` の結果)と `const char*`(裸のリテラル)が混在していると単一の `T` を決められない
- **対処**: 型を明示できる場所に渡す。このリポジトリでは `drawLines(std::initializer_list<String> lines)` のように**引数の型を明示した関数**へ渡せば、渡す側は各要素が暗黙変換されるので混在していても問題ない。新規に range-based for を書く場合は全要素を `String(...)` で揃えるより、既存の `drawLines()` を再利用するほうが簡潔
- **出典**: issue 00004(`src/main.cpp` のポータル画面実装)
