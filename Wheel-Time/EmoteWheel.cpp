#include "EmoteWheel.h"
#include <QPainter>
#include <QMouseEvent>
#include <QtMath>
#include <QPainterPath>


EmoteWheel::EmoteWheel(QWidget *parent) : QWidget(parent)
{
    setFixedSize(300, 300);
    setMouseTracking(true);
    setAttribute(Qt::WA_TranslucentBackground); // for round shape on a transparent bg
}

void EmoteWheel::addEmote(const QString &name, const QPixmap &icon)
{
    m_emotes.append({name, icon});
    update();
}

int EmoteWheel::segmentAtAngle(double angleDeg) const
{
    if (m_emotes.isEmpty()) return -1;
    double segSize = 360.0 / m_emotes.size();
    int idx = static_cast<int>(angleDeg / segSize);
    return idx % m_emotes.size();
}

void EmoteWheel::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRectF rect(0, 0, width(), height());
    QPointF center = rect.center();
    double radius = qMin(width(), height()) / 2.0;

    int count = m_emotes.size();
    if (count == 0) return;

    double segAngle = 360.0 / count;

    for (int i = 0; i < count; ++i) {
        double startAngle = i * segAngle;

        QPainterPath path;
        path.moveTo(center);
        path.arcTo(rect, -startAngle, -segAngle); // Qt angles: counter-clockwise, 0 = 3 o'clock
        path.closeSubpath();

        p.setPen(QPen(Qt::white, 2));
        p.setBrush(i == m_hoveredIndex ? QColor(100, 150, 255, 200)
                                       : QColor(40, 40, 40, 180));
        p.drawPath(path);

        // Place icon mid-segment
        double midAngle = qDegreesToRadians(startAngle + segAngle / 2.0);
        double iconR = radius * 0.6;
        QPointF iconPos = center + QPointF(qCos(midAngle), -qSin(midAngle)) * iconR;

        QPixmap icon = m_emotes[i].icon.scaled(40, 40, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        p.drawPixmap(iconPos.x() - icon.width()/2, iconPos.y() - icon.height()/2, icon);
    }

    // Optional: empty center circle (like Discord/PUBG wheels)
    p.setBrush(QColor(20, 20, 20, 200));
    p.drawEllipse(center, radius * 0.25, radius * 0.25);
}

void EmoteWheel::mouseMoveEvent(QMouseEvent *event)
{
    QPointF center = rect().center();
    QPointF d = event->pos() - center;
    double dist = qSqrt(d.x()*d.x() + d.y()*d.y());

    if (dist < rect().width()/2.0 * 0.25) {
        m_hoveredIndex = -1; // inside dead zone
    } else {
        double angle = qRadiansToDegrees(qAtan2(-d.y(), d.x()));
        if (angle < 0) angle += 360;
        m_hoveredIndex = segmentAtAngle(angle);
    }
    update();
}

void EmoteWheel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_hoveredIndex >= 0) {
        emit emoteSelected(m_emotes[m_hoveredIndex].name);
    }
}