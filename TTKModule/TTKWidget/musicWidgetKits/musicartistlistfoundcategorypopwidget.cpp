#include "musicartistlistfoundcategorypopwidget.h"
#include "musicclickedlabel.h"
#include "musicuiobject.h"
#include "musicwidgetheaders.h"

#include <QSignalMapper>

#define ITEM_MAX_COLUMN     2
#define ITEM_LABEL_WIDTH    20

MusicArtistListFoundCategoryItem::MusicArtistListFoundCategoryItem(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet(QString());
}

void MusicArtistListFoundCategoryItem::setCategory(const MusicResultsCategory &category)
{
    m_category = category;

    QHBoxLayout *layout = new QHBoxLayout(this);
    QLabel *label = new QLabel(category.m_category, this);
    label->setStyleSheet(MusicUIObject::MQSSColorStyle03 + MusicUIObject::MQSSFontStyle03);
    label->setFixedSize(100, ITEM_LABEL_WIDTH);
    layout->addWidget(label, 0, Qt::AlignTop);

    QWidget *item = new QWidget(this);
    QGridLayout *itemlayout = new QGridLayout(item);
    itemlayout->setContentsMargins(0, 0, 0, 0);

    QSignalMapper *group = new QSignalMapper(this);
    connect(group, SIGNAL(mapped(int)), SLOT(buttonClicked(int)));

    for(int i=0; i<m_category.m_items.count(); ++i)
    {
        MusicClickedLabel *l = new MusicClickedLabel(m_category.m_items[i].m_name, item);
        l->setStyleSheet(QString("QLabel::hover{%1}").arg(MusicUIObject::MQSSColorStyle08));
        l->setFixedSize(200, ITEM_LABEL_WIDTH);
        connect(l, SIGNAL(clicked()), group, SLOT(map()));
        group->setMapping(l, i);
        itemlayout->addWidget(l, i/ITEM_MAX_COLUMN, i%ITEM_MAX_COLUMN, Qt::AlignLeft);
    }
    item->setLayout(itemlayout);

    layout->addWidget(item, 0, Qt::AlignTop);
    setLayout(layout);
}

void MusicArtistListFoundCategoryItem::buttonClicked(int index)
{
    if(0 <= index && index < m_category.m_items.count())
    {
        Q_EMIT categoryChanged(m_category.m_items[index]);
    }
}


MusicArtistListFoundCategoryPopWidget::MusicArtistListFoundCategoryPopWidget(QWidget *parent)
    : MusicToolMenuWidget(parent)
{
    initWidget();
}

void MusicArtistListFoundCategoryPopWidget::setCategory(const QString &server, QObject *obj)
{
    MusicResultsCategorys categorys;
    MusicCategoryConfigManager manager;
    manager.readConfig(MusicCategoryConfigManager::ArtistList);
    manager.readCategoryData(categorys, server);

    QVBoxLayout *layout = new QVBoxLayout(m_containWidget);
    QWidget *containWidget = new QWidget(m_containWidget);
    containWidget->setStyleSheet(MusicUIObject::MQSSBackgroundStyle17);
    QVBoxLayout *containLayout = new QVBoxLayout(containWidget);
    containWidget->setLayout(containLayout);

    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setStyleSheet(MusicUIObject::MQSSScrollBarStyle01);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setAlignment(Qt::AlignLeft);
    scrollArea->setWidget(containWidget);
    layout->addWidget(scrollArea);

    foreach(const MusicResultsCategory &category, categorys)
    {
        MusicArtistListFoundCategoryItem *item = new MusicArtistListFoundCategoryItem(this);
        connect(item, SIGNAL(categoryChanged(MusicResultsCategoryItem)), obj, SLOT(categoryChanged(MusicResultsCategoryItem)));
        item->setCategory(category);
        containLayout->addWidget(item);
    }
    m_containWidget->setLayout(layout);
}

void MusicArtistListFoundCategoryPopWidget::closeMenu()
{
    m_menu->close();
}

void MusicArtistListFoundCategoryPopWidget::popupMenu()
{
    m_menu->exec(mapToGlobal(QPoint(0, 0)));
}

void MusicArtistListFoundCategoryPopWidget::initWidget()
{
    setFixedSize(100, 30);
    setTranslucentBackground();
    setText(tr("All"));

    QString style = MusicUIObject::MQSSBorderStyle04 + MusicUIObject::MQSSBackgroundStyle17;
    setObjectName("mianWidget");
    setStyleSheet(QString("#mianWidget{%1}").arg(style));

    m_containWidget->setFixedSize(600, 370);
    m_containWidget->setObjectName("containWidget");
    m_containWidget->setStyleSheet(QString("#containWidget{%1}").arg(style));

    m_menu->setStyleSheet(MusicUIObject::MQSSMenuStyle05);

}
