#include <QTimer>
#include <QSettings>
#include <QPainter>
#include <QMenu>
#include <QPaintEvent>
#include <math.h>
#include <stdlib.h>

#include <qmmp/qmmp.h>
#include "fft.h"
#include "inlines.h"
#include "normalline.h"
#include "colorwidget.h"

NormalLine::NormalLine (QWidget *parent) : Visual (parent)
{
    m_intern_vis_data = nullptr;
    m_peaks = nullptr;
    m_x_scale = nullptr;
    m_running = false;
    m_rows = 0;
    m_cols = 0;

    for(int i=0; i<50; ++i)
    {
        m_starPoints << new StarPoint();
    }

    setWindowTitle(tr("Normal Line Widget"));
    setMinimumSize(2*300-30, 105);
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout()));

    m_starTimer = new QTimer(this);
    connect(m_starTimer, SIGNAL(timeout()), this, SLOT(starTimeout()));

    m_starAction = new QAction(tr("Star"), this);
    m_starAction->setCheckable(true);
    connect(m_starAction, SIGNAL(triggered(bool)), this, SLOT(changeStarState(bool)));

    m_peaks_falloff = 0.2;
    m_analyzer_falloff = 1.2;
    m_timer->setInterval(QMMP_VISUAL_INTERVAL);
    m_starTimer->setInterval(1000);
    m_cell_size = QSize(3, 2);

    clear();
    readSettings();
}

NormalLine::~NormalLine()
{
    qDeleteAll(m_starPoints);
    if(m_peaks)
        delete[] m_peaks;
    if(m_intern_vis_data)
        delete[] m_intern_vis_data;
    if(m_x_scale)
        delete[] m_x_scale;
}

void NormalLine::start()
{
    m_running = true;
    if(isVisible())
    {
        m_timer->start();
        m_starTimer->start();
    }
}

void NormalLine::stop()
{
    m_running = false;
    m_timer->stop();
    m_starTimer->stop();
    clear();
}

void NormalLine::clear()
{
    m_rows = 0;
    m_cols = 0;
    update();
}


void NormalLine::timeout()
{
    if(takeData(m_left_buffer, m_right_buffer))
    {
        process();
        update();
    }
}

void NormalLine::starTimeout()
{
    foreach(StarPoint *point, m_starPoints)
    {
        point->m_alpha = rand()%255;
        point->m_pt = QPoint(rand()%width(), rand()%height());
    }
}

void NormalLine::readSettings()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("NormalLine");
    m_colors = ColorWidget::readColorConfig(settings.value("colors").toString());
    m_starAction->setChecked(settings.value("show_star", false).toBool());
    m_starColor = ColorWidget::readSingleColorConfig(settings.value("star_color").toString());
    settings.endGroup();
}

void NormalLine::writeSettings()
{
    QSettings settings(Qmmp::configFile(), QSettings::IniFormat);
    settings.beginGroup("NormalLine");
    settings.setValue("colors", ColorWidget::writeColorConfig(m_colors));
    settings.setValue("show_star", m_starAction->isChecked());
    settings.setValue("star_color", ColorWidget::writeSingleColorConfig(m_starColor));
    settings.endGroup();
}

void NormalLine::changeColor()
{
    ColorWidget c;
    c.setColors(m_colors);
    if(c.exec())
    {
        m_colors = c.getColors();
    }
}

void NormalLine::changeStarState(bool state)
{
    m_starAction->setChecked(state);
    update();
}

void NormalLine::changeStarColor()
{
    ColorWidget c;
    c.setColors(QList<QColor>() << m_starColor);
    if(c.exec())
    {
        QList<QColor> colors(c.getColors());
        if(!colors.isEmpty())
        {
            m_starColor = colors.first();
            update();
        }
    }
}

void NormalLine::hideEvent(QHideEvent *)
{
    m_timer->stop();
    m_starTimer->stop();
}

void NormalLine::showEvent(QShowEvent *)
{
    if(m_running)
    {
        m_timer->start();
        m_starTimer->start();
    }
}

void NormalLine::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.fillRect(e->rect(), Qt::black);
    draw(&painter);
}

void NormalLine::contextMenuEvent(QContextMenuEvent *)
{
    QMenu menu(this);
    connect(&menu, SIGNAL(triggered(QAction*)), SLOT(writeSettings()));
    connect(&menu, SIGNAL(triggered(QAction*)), SLOT(readSettings()));

    menu.addAction("Color", this, SLOT(changeColor()));
    menu.addAction(m_starAction);
    menu.addAction("StarColor", this, SLOT(changeStarColor()));

    menu.exec(QCursor::pos());
}

void NormalLine::process()
{
    static fft_state *state = nullptr;
    if(!state)
        state = fft_init();

    const int rows = (height() - 2) / m_cell_size.height();
    const int cols = (width() - 2) / m_cell_size.width() / 2;

    if(m_rows != rows || m_cols != cols)
    {
        m_rows = rows;
        m_cols = cols;
        if(m_peaks)
            delete[] m_peaks;
        if(m_intern_vis_data)
            delete[] m_intern_vis_data;
        if(m_x_scale)
            delete[] m_x_scale;
        m_peaks = new double[m_cols * 2];
        m_intern_vis_data = new double[m_cols * 2];
        m_x_scale = new int[m_cols + 1];

        for(int i = 0; i < m_cols * 2; ++i)
        {
            m_peaks[i] = 0;
            m_intern_vis_data[i] = 0;
        }
        for(int i = 0; i < m_cols + 1; ++i)
            m_x_scale[i] = pow(pow(255.0, 1.0 / m_cols), i);
    }

    short dest_l[256];
    short dest_r[256];
    short yl, yr;
    int j, k, magnitude_l, magnitude_r;

    calc_freq (dest_l, m_left_buffer);
    calc_freq (dest_r, m_right_buffer);

    const double y_scale = (double) 1.25 * m_rows / log(256);

    for(int i = 0; i < m_cols; i++)
    {
        j = m_cols * 2 - i - 1; //mirror index
        yl = yr = 0;
        magnitude_l = magnitude_r = 0;

        if(m_x_scale[i] == m_x_scale[i + 1])
        {
            yl = dest_l[i];
            yr = dest_r[i];
        }
        for(k = m_x_scale[i]; k < m_x_scale[i + 1]; k++)
        {
            yl = qMax(dest_l[k], yl);
            yr = qMax(dest_r[k], yr);
        }

        yl >>= 7; //256
        yr >>= 7;

        if(yl)
        {
            magnitude_l = int(log (yl) * y_scale);
            magnitude_l = qBound(0, magnitude_l, m_rows);
        }
        if(yr)
        {
            magnitude_r = int(log (yr) * y_scale);
            magnitude_r = qBound(0, magnitude_r, m_rows);
        }

        m_intern_vis_data[i] -= m_analyzer_falloff * m_rows / 15;
        m_intern_vis_data[i] = magnitude_l > m_intern_vis_data[i] ? magnitude_l : m_intern_vis_data[i];

        m_intern_vis_data[j] -= m_analyzer_falloff * m_rows / 15;
        m_intern_vis_data[j] = magnitude_r > m_intern_vis_data[j] ? magnitude_r : m_intern_vis_data[j];

        m_peaks[i] -= m_peaks_falloff * m_rows / 15;
        m_peaks[i] = magnitude_l > m_peaks[i] ? magnitude_l : m_peaks[i];

        m_peaks[j] -= m_peaks_falloff * m_rows / 15;
        m_peaks[j] = magnitude_r > m_peaks[j] ? magnitude_r : m_peaks[j];
    }
}

void NormalLine::draw(QPainter *p)
{
    if(m_starAction->isChecked())
    {
        foreach(StarPoint *point, m_starPoints)
        {
            m_starColor.setAlpha(point->m_alpha);
            p->setPen(QPen(m_starColor, 3));
            p->drawPoint(point->m_pt);
        }
    }

    QLinearGradient line(0, 0, 0, height());
    for(int i=0; i<m_colors.count(); ++i)
    {
        line.setColorAt((i+1)*1.0/m_colors.count(), m_colors[i]);
    }
    p->setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    int x = 0;
    const int rdx = qMax(0, width() - 2 * m_cell_size.width() * m_cols);
    const float maxed = maxRange();

    for(int j = 0; j < m_cols * 2; ++j)
    {
        x = j * m_cell_size.width() + 1;
        if(j >= m_cols)
        {
            x += rdx; //correct right part position
        }

        int hh = m_intern_vis_data[j] * maxed *m_cell_size.height();
        p->fillRect (x, height() - hh, m_cell_size.width() - 1, hh, line);

        p->fillRect (x, height() - int(m_peaks[j] * maxed) * m_cell_size.height(),
                     m_cell_size.width() - 1, m_cell_size.height(), "Cyan");
    }
}