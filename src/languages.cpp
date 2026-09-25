#include "languages.h"

#include <QFileInfo>
#include <QList>

namespace Languages {

const LanguageDef PlainText = []() {
    LanguageDef d;
    d.name = QStringLiteral("Text");
    d.hasStrings = false;
    d.hasCharLiterals = false;
    return d;
}();

#define S(x) QStringLiteral(x)

// Split a space separated keyword list into a QStringList.
static QStringList kw(const char *words)
{
    return QString::fromLatin1(words).split(QLatin1Char(' '), Qt::SkipEmptyParts);
}

static const QList<LanguageDef> &table()
{
    static const QList<LanguageDef> defs = []() {
        QList<LanguageDef> l;

        { // C / C++
            LanguageDef d;
            d.name = S("C++");
            d.extensions = { S("c"), S("cc"), S("cpp"), S("cxx"), S("h"), S("hh"), S("hpp"), S("hxx"), S("ino"), S("tpp") };
            d.control = kw("if else for while do switch case default break continue return goto try catch throw co_return co_await co_yield");
            d.keywords = kw("alignas alignof asm and and_eq bitand bitor class compl concept const consteval constexpr constinit const_cast decltype delete dynamic_cast enum explicit export extern friend inline mutable namespace new noexcept not not_eq operator or or_eq private protected public register reinterpret_cast requires sizeof static static_assert static_cast struct template this thread_local typedef typeid typename union using virtual volatile xor xor_eq");
            d.types = kw("auto bool char char8_t char16_t char32_t double float int long short signed unsigned void wchar_t");
            d.literals = kw("true false nullptr NULL");
            d.lineComments = { S("//") };
            d.blockCommentStart = S("/*");
            d.blockCommentEnd = S("*/");
            d.preprocessor = true;
            l << d;
        }
        { // C#
            LanguageDef d;
            d.name = S("C#");
            d.extensions = { S("cs"), S("csx") };
            d.control = kw("if else for foreach while do switch case default break continue return goto try catch finally throw yield checked unchecked lock");
            d.keywords = kw("abstract as base bool? class const delegate event explicit implicit in interface internal is nameof namespace new operator out override params partial readonly ref sealed sizeof stackalloc static struct this typeof unchecked unsafe using var virtual when where with get set init add remove");
            d.types = kw("bool byte char decimal double dynamic float int long object sbyte short string uint ulong ushort void var");
            d.literals = kw("true false null");
            d.lineComments = { S("//") };
            d.blockCommentStart = S("/*");
            d.blockCommentEnd = S("*/");
            d.preprocessor = true;
            l << d;
        }
        { // Java
            LanguageDef d;
            d.name = S("Java");
            d.extensions = { S("java") };
            d.control = kw("if else for while do switch case default break continue return try catch finally throw synchronized");
            d.keywords = kw("abstract assert class const enum extends final implements import instanceof interface native new package private protected public static strictfp super this throws transient volatile var record sealed permits yield");
            d.types = kw("boolean byte char double float int long short void");
            d.literals = kw("true false null");
            d.lineComments = { S("//") };
            d.blockCommentStart = S("/*");
            d.blockCommentEnd = S("*/");
            l << d;
        }
        { // JavaScript / TypeScript
            LanguageDef d;
            d.name = S("JavaScript");
            d.extensions = { S("js"), S("jsx"), S("mjs"), S("cjs"), S("ts"), S("tsx") };
            d.control = kw("if else for while do switch case default break continue return try catch finally throw with yield await");
            d.keywords = kw("async class const debugger delete export extends from function get let of set static super this typeof void new instanceof in import as");
            d.types = kw("any bigint boolean declare enum implements interface keyof namespace never number object private protected public readonly string symbol type unknown");
            d.literals = kw("true false null undefined NaN Infinity");
            d.lineComments = { S("//") };
            d.blockCommentStart = S("/*");
            d.blockCommentEnd = S("*/");
            d.backtickStrings = true;
            l << d;
        }
        { // Python
            LanguageDef d;
            d.name = S("Python");
            d.extensions = { S("py"), S("pyw"), S("pyi") };
            d.control = kw("and as assert async await break continue elif else except finally for from global if import in is lambda nonlocal not or pass raise return try while with yield match case");
            d.keywords = kw("del");
            d.types = kw("bool bytearray bytes complex dict float frozenset int list set str tuple type object");
            d.literals = kw("True False None self cls");
            d.lineComments = { S("#") };
            d.hasCharLiterals = false;
            d.tripleQuotedStrings = true;
            d.decorator = true;
            l << d;
        }
        { // JSON
            LanguageDef d;
            d.name = S("JSON");
            d.extensions = { S("json"), S("jsonc"), S("geojson"), S("har") };
            d.literals = kw("true false null");
            d.hasCharLiterals = false;
            l << d;
        }
        { // HTML / XML
            LanguageDef d;
            d.name = S("HTML");
            d.extensions = { S("html"), S("htm"), S("xhtml"), S("xml"), S("svg"), S("ui"), S("plist"), S("qml") };
            d.lineComments = {}; // <!-- --> handled as block comment
            d.blockCommentStart = S("<!--");
            d.blockCommentEnd = S("-->");
            d.markup = true;
            d.hasCharLiterals = false;
            l << d;
        }
        { // CSS
            LanguageDef d;
            d.name = S("CSS");
            d.extensions = { S("css"), S("scss"), S("less") };
            d.lineComments = {}; // scss has //, keep it simple: block comments only
            d.blockCommentStart = S("/*");
            d.blockCommentEnd = S("*/");
            d.hasCharLiterals = false;
            d.atRules = true;
            d.hashColors = true;
            l << d;
        }
        { // INI
            LanguageDef d;
            d.name = S("INI");
            d.extensions = { S("ini"), S("cfg"), S("conf"), S("properties"), S("editorconfig"), S("gitconfig"), S("toml") };
            d.lineComments = { S(";"), S("#") };
            d.hasStrings = false;
            d.hasCharLiterals = false;
            d.ini = true;
            l << d;
        }
        { // Bash
            LanguageDef d;
            d.name = S("Shell");
            d.extensions = { S("sh"), S("bash"), S("zsh"), S("command") };
            d.control = kw("if then elif else fi for while until do done case esac in select time coproc");
            d.keywords = kw("break continue export function local readonly return shift source alias unset typeset declare eval exec exit read cd pushd popd");
            d.literals = kw("true false");
            d.lineComments = { S("#") };
            l << d;
        }
        { // CMake
            LanguageDef d;
            d.name = S("CMake");
            d.extensions = { S("cmake") };
            d.control = kw("if elseif else endif foreach endforeach while endwhile function endfunction macro endmacro");
            d.keywords = kw("set list file message include project add_executable add_library target_link_libraries target_include_directories target_compile_definitions target_sources find_package find_library find_path find_program option cmake_minimum_required cmake_policy return break continue define_property separate_arguments string math unset mark_as_advanced get_filename_path install add_custom_command add_custom_target add_subdirectory add_test enable_testing set_target_properties set_property set_source_files_properties get_target_property include_directories link_directories install targets export");
            d.literals = kw("ON OFF TRUE FALSE");
            d.lineComments = { S("#") };
            d.hasStrings = true;
            d.hasCharLiterals = false;
            l << d;
        }
        { // Go
            LanguageDef d;
            d.name = S("Go");
            d.extensions = { S("go") };
            d.control = kw("break case continue default defer else fallthrough for go goto if range return select switch");
            d.keywords = kw("chan const func import interface map package struct type var");
            d.types = kw("bool byte complex64 complex128 error float32 float64 int int8 int16 int32 int64 rune string uint uint8 uint16 uint32 uint64 uintptr any");
            d.literals = kw("true false nil iota");
            d.lineComments = { S("//") };
            d.blockCommentStart = S("/*");
            d.blockCommentEnd = S("*/");
            l << d;
        }
        { // Rust
            LanguageDef d;
            d.name = S("Rust");
            d.extensions = { S("rs") };
            d.control = kw("as async await break continue dyn else for if in loop match return unsafe while yield");
            d.keywords = kw("crate enum extern false fn impl let mod move mut pub ref self static struct super trait true type union use where");
            d.types = kw("bool char f32 f64 i8 i16 i32 i64 i128 isize str u8 u16 u32 u64 u128 usize String Vec Box Option Result");
            d.literals = kw("true false None Some Ok Err");
            d.lineComments = { S("//") };
            d.blockCommentStart = S("/*");
            d.blockCommentEnd = S("*/");
            d.attributeBracket = true;
            l << d;
        }
        return l;
    }();
    return defs;
}

const LanguageDef *detect(const QString &fileName)
{
    const QString base = QFileInfo(fileName).fileName();
    const QString lower = base.toLower();

    if (lower == S("cmakelists.txt"))
        return byName(S("CMake"));
    if (lower == S("makefile") || lower.startsWith(S("makefile.")))
        return &PlainText;

    const QString suffix = QFileInfo(fileName).suffix().toLower();
    if (!suffix.isEmpty()) {
        for (const LanguageDef &d : table()) {
            if (d.extensions.contains(suffix))
                return &d;
        }
    }
    return &PlainText;
}

const LanguageDef *byName(const QString &name)
{
    if (name == PlainText.name)
        return &PlainText;
    for (const LanguageDef &d : table()) {
        if (d.name == name)
            return &d;
    }
    return nullptr;
}

const QList<LanguageDef> &all()
{
    return table();
}

} // namespace Languages
