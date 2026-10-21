#include "Parser.h"
#include "Expansion.h"
#include <QDir>
namespace terlox::shell {
struct Token { QString text; bool op = false; };
ParseResult parse(const QString& line, int status) {
    ParseResult result;
    std::vector<Token> tokens;
    QString word; QChar quote; bool started = false;
    auto flush = [&] { if (started) tokens.push_back({word, false}); word.clear(); started = false; };
    try {
        for (qsizetype i = 0; i < line.size(); ++i) {
            const auto c = line[i];
#ifdef Q_OS_WIN
            if (c=='%' && quote!='\'') {
                const auto end=line.indexOf('%',i+1);
                if (end>i+1) {
                    const auto name=line.mid(i+1,end-i-1);
                    if (qEnvironmentVariableIsSet(name.toUtf8().constData())) { word+=qEnvironmentVariable(name.toUtf8().constData()); i=end; started=true; continue; }
                }
            }
#endif
            if (c == '\\' && quote != '\'') {
#ifdef Q_OS_WIN
                // Windows quoted paths use literal separators, including before
                // the closing quote. Drive roots and UNC paths are not escapes.
                const bool windowsPath=(word.size()>=2 && word[0].isLetter() && word[1]==':') || word.startsWith("\\");
                if (quote=='"' || windowsPath || i+1==line.size() || (word.isEmpty() && line[i+1]=='\\')) { word+=c; started=true; continue; }
#endif
                if (i + 1 == line.size()) throw QString("trailing escape");
                const auto next = line[i + 1];
                // Preserve ordinary Windows path separators; escape shell metacharacters.
                if (QString("\\\"' $|;<>\t").contains(next)) { word += next; ++i; }
                else word += c;
                started = true; continue;
            }
            if (!quote.isNull()) {
                if (c == quote) quote = QChar();
                else if (c == '$' && quote == '"') word += expandVariable(line, i, status);
                else word += c;
                continue;
            }
            if (c == '#' && !started) break;
            if (c == '\'' || c == '"') { quote = c; started = true; continue; }
            if (c.isSpace()) { flush(); continue; }
            if (c == '&') throw QString("background jobs and && are not supported");
            if (QString("|;<>").contains(c)) {
                QString op(c);
                if (c == '>' && word == "2") { word.clear(); started = false; op = "2>"; }
                flush();
                if (c == '>' && i + 1 < line.size() && line[i + 1] == '>') { op += '>'; ++i; }
                tokens.push_back({op, true}); continue;
            }
            if (c == '$') word += expandVariable(line, i, status);
            else if (c == '~' && !started && (i + 1 == line.size() || line[i + 1] == '/')) word += QDir::homePath();
            else word += c;
            started = true;
        }
        if (!quote.isNull()) throw QString("unclosed quote");
        flush();
        if (tokens.empty()) return result;
        Pipeline pipeline; Command command;
        auto finish = [&] {
            if (command.arguments.isEmpty()) throw QString("expected a command");
            pipeline.push_back(command); command = {};
        };
        for (size_t i = 0; i < tokens.size(); ++i) {
            const auto& t = tokens[i];
            if (!t.op) { command.arguments << t.text; continue; }
            if (t.text == "|" || t.text == ";") {
                finish();
                if (t.text == ";") { result.program.push_back(pipeline); pipeline.clear(); }
            } else {
                if (++i >= tokens.size() || tokens[i].op) throw QString("redirection requires a filename");
                const auto path = tokens[i].text;
                if (t.text == "<") command.input = path;
                else if (t.text == "2>") command.error = path;
                else if (t.text == ">" || t.text == ">>") { command.output = path; command.append = t.text == ">>"; }
                else throw QString("unsupported redirection: ") + t.text;
            }
        }
        if (!command.arguments.isEmpty()) finish();
        else if (!pipeline.empty()) throw QString("expected a command after pipe");
        if (!pipeline.empty()) result.program.push_back(pipeline);
    } catch (const QString& e) { result.error = e; result.program.clear(); }
    return result;
}
}
