#include "ProjectFiles.h"
#include "AtomicFile.h"
#include <juce_cryptography/juce_cryptography.h>
#define MINIZ_NO_ZLIB_COMPATIBLE_NAMES
#include <miniz.h>

namespace rocker {
using namespace juce;
namespace {
size_t readZip(void* source,mz_uint64 pos,void* data,size_t bytes){
    auto& in=*static_cast<InputStream*>(source);
    if(pos>mz_uint64(std::numeric_limits<int64>::max()) || !in.setPosition((int64)pos))return 0;
    return size_t(std::max(0,in.read(data,int(std::min<size_t>(bytes,std::numeric_limits<int>::max())))));
}
size_t writeZip(void* dest,mz_uint64 pos,const void* data,size_t bytes){
    auto& out=*static_cast<OutputStream*>(dest);
    if(pos>mz_uint64(std::numeric_limits<int64>::max()) || (out.getPosition()!=(int64)pos && !out.setPosition((int64)pos)))return 0;
    return out.write(data,bytes)?bytes:0;
}
struct Archive{
    mz_zip_archive zip{};
    ~Archive(){mz_zip_end(&zip);}
};
bool validName(const String& name){
    if(name=="skladba.json" || name=="zobrazeni.json" || name=="soucty.json")return true;
    auto folder=name.upToFirstOccurrenceOf("/",false,false);
    auto leaf=name.fromFirstOccurrenceOf("/",false,false);
    if(leaf.isEmpty() || leaf.containsAnyOf("/\\:*?\"<>|") || leaf.startsWithChar('.') ||
       leaf.endsWithChar('.') || leaf.endsWithChar(' ') || File::createLegalFileName(leaf)!=leaf)return false;
    const auto base=leaf.upToFirstOccurrenceOf(".",false,false).toUpperCase();
    static const StringArray reserved{"CON","PRN","AUX","NUL","COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9",
        "LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"};
    if(reserved.contains(base))return false;
    if(folder=="Zvuk")return leaf.endsWithIgnoreCase(".wav") || leaf.endsWithIgnoreCase(".mid");
    if(folder=="Verze")return leaf.endsWithIgnoreCase(".json");
    return folder=="Nahledy";
}
struct Scratch{
    File dir;
    ~Scratch(){if(dir.isDirectory())dir.deleteRecursively();}
};
bool validated(const File& dir){
    bool damaged=false;
    const auto song=Song::parse(JSON::parse(dir.getChildFile("skladba.json")),damaged);
    if(!song || damaged)return false;
    for(auto& a:song->assets)if(!dir.getChildFile(a.path).existsAsFile())return false;
    return true;
}
}
bool packSong(const File& project,const File& target){
    if(!validated(project) || target.isDirectory())return false;
    TemporaryFile temp(target);
    auto output=temp.getFile().createOutputStream();if(!output)return false;
    Archive archive;archive.zip.m_pWrite=writeZip;archive.zip.m_pIO_opaque=static_cast<OutputStream*>(output.get());
    if(!mz_zip_writer_init_v2(&archive.zip,0,MZ_ZIP_FLAG_WRITE_ZIP64))return false;
    auto* hashes=new DynamicObject;
    var manifest(hashes);

    auto add=[&](const File& file){
        auto name=file.getRelativePathFrom(project).replaceCharacter('\\','/');
        if(!validName(name) || file.isSymbolicLink() || file==target || hashes->hasProperty(name))return false;
        FileInputStream stream(file);if(!stream.openedOk())return false;
        hashes->setProperty(name,SHA256(file).toHexString());
        return mz_zip_writer_add_read_buf_callback(&archive.zip,name.toRawUTF8(),readZip,
            static_cast<InputStream*>(&stream),mz_uint64(file.getSize()),nullptr,nullptr,0,0,nullptr,0,nullptr,0)!=0;
    };
    if(!add(project.getChildFile("skladba.json")))return false;
    auto view=project.getChildFile("zobrazeni.json");if(view.existsAsFile() && !add(view))return false;
    for(auto folder:{"Zvuk","Verze","Nahledy"}){
        auto dir=project.getChildFile(folder);
        if(dir.isSymbolicLink())return false;
        for(auto file:dir.findChildFiles(File::findFiles,false))if(!add(file))return false;
    }
    auto json=JSON::toString(manifest);
    if(!mz_zip_writer_add_mem(&archive.zip,"soucty.json",json.toRawUTF8(),json.getNumBytesAsUTF8(),6) ||
       !mz_zip_writer_finalize_archive(&archive.zip))return false;
    output->flush();if(output->getStatus().failed())return false;
    output.reset();
    return commitTemporary(temp);
}
std::optional<File> unpackSong(const File& package,const File& library){
    FileInputStream source(package);if(!source.openedOk() || library.createDirectory().failed())return {};
    Archive archive;archive.zip.m_pRead=readZip;archive.zip.m_pIO_opaque=static_cast<InputStream*>(&source);
    if(!mz_zip_reader_init(&archive.zip,mz_uint64(source.getTotalLength()),0))return {};
    auto count=mz_zip_reader_get_num_files(&archive.zip);if(count<2)return {};
    StringArray entries;
    Scratch temp{library.getChildFile(".rozbaleni-"+Uuid().toString())};
    if(temp.dir.createDirectory().failed())return {};
    std::set<String> seen;
    int64 remaining=library.getBytesFreeOnVolume();
    for(mz_uint i=0;i<count;++i){
        mz_zip_archive_file_stat info{};
        auto length=mz_zip_reader_get_filename(&archive.zip,i,nullptr,0);
        if(length<2 || length>65536 || !mz_zip_reader_file_stat(&archive.zip,i,&info) || info.m_is_directory ||
            info.m_is_encrypted || (info.m_external_attr>>16 & 0170000)==0120000 || !info.m_is_supported)return {};
        std::vector<char> raw(length);
        if(mz_zip_reader_get_filename(&archive.zip,i,raw.data(),length)!=length)return {};
        auto name=String::fromUTF8(raw.data(),int(length)-1);
        if(name.getNumBytesAsUTF8()+1!=length || !validName(name) || !seen.insert(name.toLowerCase()).second ||
            (remaining>=0 && info.m_uncomp_size>mz_uint64(remaining)))return {};
        if(remaining>=0)remaining-=int64(info.m_uncomp_size);
        entries.add(name);
        auto file=temp.dir.getChildFile(name);
        if(!file.isAChildOf(temp.dir) || file.getParentDirectory().createDirectory().failed())return {};
        FileOutputStream out(file);
        if(!out.openedOk() || !mz_zip_reader_extract_to_callback(&archive.zip,i,writeZip,static_cast<OutputStream*>(&out),0) ||
           mz_uint64(out.getPosition())!=info.m_uncomp_size)return {};
        out.flush();if(out.getStatus().failed())return {};
    }
    auto sums=temp.dir.getChildFile("soucty.json");
    if(sums.getSize()>16*1024*1024)return {};
    auto json=JSON::parse(sums);
    auto* hashes=json.getDynamicObject();
    if(!hashes || hashes->getProperties().size()!=count-1)return {};
    for(auto& name:entries){if(name=="soucty.json")continue;
        if(hashes->getProperty(name).toString()!=SHA256(temp.dir.getChildFile(name)).toHexString())return {};}
    if(!validated(temp.dir) || !sums.deleteFile())return {};
    auto title=JSON::parse(temp.dir.getChildFile("skladba.json")).getProperty("nazev",{}).toString();
    auto leaf=File::createLegalFileName(title).removeCharacters(".").trim();
    if(leaf.isEmpty())leaf=Uuid().toString();
    auto dest=library.getNonexistentChildFile(leaf,{},false);
    if(!temp.dir.moveFileTo(dest))return {};
    return dest;
}
bool syncedFolder(const File& dir){
    for(auto key:{"OneDrive","OneDriveConsumer","OneDriveCommercial"}){
        auto raw=SystemStats::getEnvironmentVariable(key,{});
        if(raw.isNotEmpty() && File::isAbsolutePath(raw) && (dir==File(raw) || dir.isAChildOf(File(raw))))return true;
    }
    auto parts=StringArray::fromTokens(dir.getFullPathName(),"/\\",{});
    for(auto& part:parts)if(part.equalsIgnoreCase("OneDrive") || part.startsWithIgnoreCase("OneDrive - "))return true;
    return false;
}
std::set<String> usedAudio(const Song& song){
    std::set<String> ids, paths;
    auto gather=[&](auto& clips,auto& groups){
        for(auto& c:clips)ids.insert(c.asset);
        for(auto& group:groups)for(auto& take:group.attempts)for(auto& c:take.clips)ids.insert(c.asset);
    };
    gather(song.clips,song.groups);
    for(auto& [key,section]:song.sections)gather(section.clips,section.groups);
    for(auto& a:song.assets)if(ids.contains(a.id))paths.insert(a.path);
    return paths;
}
Array<File> unusedAudio(const Song& song,const File& dir,const EditHistory& history){
    Array<File> result;
    if(!dir.getChildFile("Zachrana").findChildFiles(File::findFiles,false).isEmpty())return result;
    auto keep=usedAudio(song);
    history.visit([&](const Song& snapshot){keep.merge(usedAudio(snapshot));});
    for(auto& file:dir.getChildFile("Verze").findChildFiles(File::findFiles,false,"*.json")){
        bool damaged=false;auto snapshot=Song::parse(JSON::parse(file),damaged);
        if(!snapshot || damaged)return {};
        // A restored version may load any asset listed in its catalog.
        for(auto& a:snapshot->assets)keep.insert(a.path);
    }
    auto audio=dir.getChildFile("Zvuk");if(audio.isSymbolicLink())return {};
    for(auto& file:audio.findChildFiles(File::findFiles,false)){
        auto name="Zvuk/"+file.getFileName();
        if(validName(name) && !file.isSymbolicLink() && !keep.contains(name))result.add(file);
    }
    return result;
}
int trashAudio(Song& song,const File& dir,const EditHistory& history,const std::function<bool(const File&)>& trash){
    const auto candidates=unusedAudio(song,dir,history);
    if(candidates.isEmpty())return 0;
    auto next=song;
    std::erase_if(next.assets,[&](auto& a){return candidates.contains(dir.getChildFile(a.path));});
    if(!next.save(dir))return -1;
    song=std::move(next);
    int count=0;
    for(auto& file:candidates)if(trash(file))++count;
    return count==candidates.size()?count:-1;
}
} // namespace rocker
