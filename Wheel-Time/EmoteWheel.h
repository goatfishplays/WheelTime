#pragma once
#include <QWidget>
#include <QVector>
#include <QPixmap>

class EmoteWheel : public QWidget
{
    Q_OBJECT
public:
    explicit EmoteWheel(QWidget *parent = nullptr);
    void addEmote(const QString &name, const QPixmap &icon);

signals:
    void emoteSelected(const QString &name);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    struct Emote { QString name; QPixmap icon; };
    QVector<Emote> m_emotes;
    int m_hoveredIndex = -1;

    int segmentAtAngle(double angleDeg) const;
};