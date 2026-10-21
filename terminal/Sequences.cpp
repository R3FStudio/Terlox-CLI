#include "Sequences.h"
namespace terlox {
void Sequences::feed(const QByteArray& bytes,const std::function<void(const QByteArray&)>& text,const std::function<bool(const QByteArray&)>& extension) {
    QByteArray plain;
    auto flush=[&] { if (!plain.isEmpty()) { text(plain); plain.clear(); } };
    auto finish=[&] { if (!dropping_ && !extension(pending_)) text(pending_); pending_.clear(); state_=Normal; escaped_=dropping_=false; };
    for (const auto ch:bytes) {
        const auto c=static_cast<unsigned char>(ch);
        if (state_==Normal) { if (c!=27) plain+=ch; else { flush(); pending_="\033"; state_=Escape; } continue; }
        if (!dropping_) pending_+=ch;
        if (state_==Escape) {
            if (ch=='[') state_=Csi;
            else if (ch==']' || ch=='_' || ch=='P' || ch=='^' || ch=='X') { state_=String; stringKind_=ch; }
            else if (c>=0x30 && c<=0x7e) finish();
        } else if (state_==Csi) { if (c>=0x40 && c<=0x7e) finish(); }
        else if (state_==String) {
            const bool end=(ch=='\a' && stringKind_==']') || (escaped_ && ch=='\\');
            escaped_=c==27; if (end) finish();
        }
        if (pending_.size()>24*1024*1024) { pending_.clear(); dropping_=true; }
        if (c==0x18 || c==0x1a) { pending_.clear(); state_=Normal; escaped_=dropping_=false; }
    }
    flush();
}
}
