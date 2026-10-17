#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <locale>
#include <cmath>
#include <cstdint>
#include <limits>
namespace HorizonGroupsContract
{
// Strict UTF-8 JSON grammar, preserving numeric lexemes and every character of a key.
class JsonObject
{
    const std::string& S;size_t I=0;std::string Out;int MaxDepth=16;size_t MaxBytes=32*1024*1024;
    std::vector<std::string>* Objects=nullptr;std::string Marker;bool Mask=false;std::vector<std::string> Fields={"data"};size_t ObjectLimit=std::numeric_limits<size_t>::max();
    void Space(){while(I<S.size()&&(S[I]==' '||S[I]=='\n'||S[I]=='\r'||S[I]=='\t'))++I;}
    bool Hex(uint32_t& N){N=0;for(int K=0;K<4;++K){if(I>=S.size())return false;unsigned char C=S[I++];int D=C>='0'&&C<='9'?C-'0':C>='a'&&C<='f'?C-'a'+10:C>='A'&&C<='F'?C-'A'+10:-1;if(D<0)return false;N=N*16+D;}return true;}
    void Character(uint32_t U)
    {
        if(U=='"'||U=='\\'){Out+='\\';Out+=char(U);}
        else if(U=='\b')Out+="\\b";else if(U=='\f')Out+="\\f";else if(U=='\n')Out+="\\n";else if(U=='\r')Out+="\\r";else if(U=='\t')Out+="\\t";
        else if(U<32){const char* H="0123456789abcdef";Out+="\\u00";Out+=H[U>>4];Out+=H[U&15];}
        else if(U<128)Out+=char(U);else if(U<2048){Out+=char(0xc0|(U>>6));Out+=char(0x80|(U&63));}
        else if(U<65536){Out+=char(0xe0|(U>>12));Out+=char(0x80|((U>>6)&63));Out+=char(0x80|(U&63));}
        else {Out+=char(0xf0|(U>>18));Out+=char(0x80|((U>>12)&63));Out+=char(0x80|((U>>6)&63));Out+=char(0x80|(U&63));}
    }
    bool String()
    {
        ++I;Out+='"';while(I<S.size())
        {
            unsigned char C=S[I++];if(C=='"'){Out+='"';return true;}if(C<32)return false;uint32_t U=C;
            if(C=='\\')
            {
                if(I>=S.size())return false;C=S[I++];
                if(C=='u'){if(!Hex(U))return false;if(U>=0xd800&&U<=0xdbff){if(I+2>S.size()||S[I++]!='\\'||S[I++]!='u')return false;uint32_t L;if(!Hex(L)||L<0xdc00||L>0xdfff)return false;U=0x10000+((U-0xd800)<<10)+(L-0xdc00);}else if(U>=0xdc00&&U<=0xdfff)return false;}
                else if(C=='"'||C=='\\'||C=='/')U=C;else if(C=='b')U='\b';else if(C=='f')U='\f';else if(C=='n')U='\n';else if(C=='r')U='\r';else if(C=='t')U='\t';else return false;
            }
            else if(C>=128)
            {
                int N=C>=0xc2&&C<=0xdf?1:C>=0xe0&&C<=0xef?2:C>=0xf0&&C<=0xf4?3:-1;if(N<0||I+N>S.size())return false;U=C&((1<<(6-N))-1);
                for(int K=0;K<N;++K){unsigned char B=S[I++];if((B&0xc0)!=0x80)return false;U=(U<<6)|(B&63);}
                if((N==1&&U<0x80)||(N==2&&U<0x800)||(N==3&&U<0x10000)||U>0x10ffff||(U>=0xd800&&U<=0xdfff))return false;
            }
            Character(U);
        }return false;
    }
    bool Value(int Depth)
    {
        if(Depth>MaxDepth)return false;Space();if(I>=S.size())return false;char C=S[I];if(C=='"')return String();
        if(C=='{'||C=='[')
        {
            bool Object=C=='{';char End=Object?'}':']';Out+=S[I++];Space();if(I<S.size()&&S[I]==End){Out+=S[I++];return true;}
            while(true)
            {
                bool Data=false;
                if(Object){Space();size_t Key=Out.size();if(I>=S.size()||S[I]!='"'||!String())return false;if(Mask)for(const auto& Field:Fields)if(Out.substr(Key)=="\""+Field+"\"")Data=true;Space();if(I>=S.size()||S[I++]!=':')return false;Out+=':';}
                Space();const size_t Start=Out.size();const bool Capture=Data&&I<S.size()&&S[I]=='{';bool WasMask=Mask;if(Capture)Mask=false;
                if(!Value(Depth+1))return false;Mask=WasMask;
                if(Capture){if(Objects->size()>=ObjectLimit)return false;Objects->push_back(Out.substr(Start));Out.resize(Start);Out+='"';Out+=Marker+std::to_string(Objects->size()-1);Out+='"';}
                Space();if(I>=S.size())return false;if(S[I]==End){Out+=S[I++];return true;}if(S[I++]!=',')return false;Out+=',';
            }
        }
        for(const char* Literal:{"true","false","null"}){size_t N=std::char_traits<char>::length(Literal);if(S.compare(I,N,Literal)==0){Out+=Literal;I+=N;return true;}}
        size_t Start=I;if(S[I]=='-')++I;if(I>=S.size())return false;if(S[I]=='0')++I;else{if(S[I]<'1'||S[I]>'9')return false;while(I<S.size()&&S[I]>='0'&&S[I]<='9')++I;}
        if(I<S.size()&&S[I]=='.'){++I;size_t First=I;while(I<S.size()&&S[I]>='0'&&S[I]<='9')++I;if(I==First)return false;}
        if(I<S.size()&&(S[I]=='e'||S[I]=='E')){++I;if(I<S.size()&&(S[I]=='+'||S[I]=='-'))++I;size_t First=I;while(I<S.size()&&S[I]>='0'&&S[I]<='9')++I;if(I==First)return false;}
        std::string Number=S.substr(Start,I-Start);std::istringstream Stream(Number);Stream.imbue(std::locale::classic());double D=0;Stream>>D;if((Stream.fail()&&D!=0)||!std::isfinite(D))return false;Out+=Number;return true;
    }
public:
    explicit JsonObject(const std::string& Input,int Depth=16,size_t Bytes=32*1024*1024):S(Input),MaxDepth(Depth),MaxBytes(Bytes){}
    bool Parse(std::string& Compact){Space();if(S.size()>MaxBytes||I>=S.size()||S[I]!='{'||!Value(0))return false;Space();if(I!=S.size())return false;Compact=Out;return true;}
    bool MaskObjects(const std::string& Prefix,std::vector<std::string>& Raw,std::string& Sanitized,const std::vector<std::string>& Names={"data"},size_t Limit=std::numeric_limits<size_t>::max()){MaxDepth=1024;Objects=&Raw;Marker=Prefix;Fields=Names;ObjectLimit=Limit;Mask=true;return Parse(Sanitized);}
};
inline bool CompactObject(const std::string& Raw,std::string& Compact,int Depth=16,size_t Bytes=32*1024*1024){return JsonObject(Raw,Depth,Bytes).Parse(Compact);}
}
