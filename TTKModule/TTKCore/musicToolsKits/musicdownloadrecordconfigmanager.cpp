#include "musicdownloadrecordconfigmanager.h"

MusicDownloadRecordConfigManager::MusicDownloadRecordConfigManager(Type type, QObject *parent)
    : MusicAbstractXml(parent)
{
    m_type = type;
}

QString MusicDownloadRecordConfigManager::getClassName()
{
    return staticMetaObject.className();
}

void MusicDownloadRecordConfigManager::writeDownloadConfig(const MusicDownloadRecords &records)
{
    if( !writeConfig( mappingFilePathFromEnum() ) )
    {
        return;
    }
    ///////////////////////////////////////////////////////
    createProcessingInstruction();
    QDomElement musicPlayer = createRoot(APPNAME);
    QDomElement download = writeDom(musicPlayer, "download");

    foreach(const MusicDownloadRecord &record, records)
    {
        writeDomElementMutilText(download, "value", MusicXmlAttributes() << MusicXmlAttribute("name", record.m_name)
                                                 << MusicXmlAttribute("size", record.m_size)
                                                 << MusicXmlAttribute("time", record.m_time), record.m_path);
    }

    //Write to file
    QTextStream out(m_file);
    m_document->save(out, 4);
}

void MusicDownloadRecordConfigManager::readDownloadConfig(MusicDownloadRecords &records)
{
    QDomNodeList nodelist = m_document->elementsByTagName("value");
    for(int i=0; i<nodelist.count(); ++i)
    {
        MusicDownloadRecord record;
        record.m_name = nodelist.at(i).toElement().attribute("name");
        record.m_size = nodelist.at(i).toElement().attribute("size");
        record.m_time = nodelist.at(i).toElement().attribute("time");
        record.m_path = nodelist.at(i).toElement().text();
        records << record;
    }
}

QString MusicDownloadRecordConfigManager::mappingFilePathFromEnum() const
{
    switch(m_type)
    {
        case Normal: return NORMALDOWNPATH_FULL;
        case Cloud : return CLOUDDOWNPATH_FULL;
        default: return QString();
    }
}
